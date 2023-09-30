#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <gtk/gtk.h>

#include "bslib.h"
#include "expense.h"
#include "expenseui.h"

void quit(const char *s);
void print_error(const char *s);
void panic(const char *s);
static void setup_ui(ExpContext *ctx);
static GtkWidget *create_menubar(ExpContext *ctx, GtkWidget *mainwin);
static int open_expense_file(ExpContext *ctx, char *xpfile);

static void file_open(GtkWidget *w, gpointer data);

int main(int argc, char *argv[]) {
    ExpContext *ctx;
    int z;

    ctx = create_context();
    gtk_init(&argc, &argv);
    setup_ui(ctx);

    if (argc > 1) {
        z = open_expense_file(ctx, argv[1]);
        if (z != 0)
            print_error("Error reading expense file");
    }

    gtk_main();
    free_context(ctx);
    return 0;
}

void quit(const char *s) {
    if (s)
        printf("%s\n", s);
    exit(0);
}
void print_error(const char *s) {
    if (s)
        fprintf(stderr, "%s: %s\n", s, strerror(errno));
    else
        fprintf(stderr, "%s\n", strerror(errno));
}
void panic(const char *s) {
    print_error(s);
    exit(1);
}

static void setup_ui(ExpContext *ctx) {
    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *statusbar;
    GtkWidget *notebook;
    GtkWidget *sw_xps;
    GtkWidget *vbox1;
    GtkWidget *hbox1;

    // mainwin
    mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainwin), "gtktest");
    gtk_window_set_default_size(GTK_WINDOW(mainwin), 640, 480);
    gtk_widget_set_size_request(mainwin, 800, 600);

    menubar = create_menubar(ctx, mainwin);
    statusbar = gtk_statusbar_new();
    gtk_widget_set_halign(statusbar, GTK_ALIGN_END);

    guint statusid =  gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "info");
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, "Expense Buddy GUI");

    sw_xps = create_expenses_treeview(ctx);
    hbox1 = create_filter_section(ctx);

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);
    g_object_set(notebook, "margin-start", 10, "margin-end", 10, NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw_xps, gtk_label_new("Expenses"));

    // Main window
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox1), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), notebook, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), statusbar, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(mainwin), vbox1);
    gtk_widget_show_all(mainwin);

    g_signal_connect(mainwin, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_grab_focus(ctx->tv_xps);

    ctx->mainwin = mainwin;
    ctx->menubar = menubar;
    ctx->notebook = notebook;
}

static GtkWidget *create_menubar(ExpContext *ctx, GtkWidget *mainwin) {
    GtkWidget *menubar;
    GtkWidget *filemenu, *mi_file, *mi_file_new, *mi_file_open, *mi_file_quit;
    GtkAccelGroup *accel;

    menubar = gtk_menu_bar_new();
    filemenu = gtk_menu_new();
    mi_file = gtk_menu_item_new_with_mnemonic("_File");
    mi_file_new = gtk_menu_item_new_with_mnemonic("_New");
    mi_file_open = gtk_menu_item_new_with_mnemonic("_Open");
    mi_file_quit = gtk_menu_item_new_with_mnemonic("_Quit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_file), filemenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), mi_file_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), mi_file_open);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), mi_file_quit);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), mi_file);

    g_signal_connect(mi_file_open, "activate", G_CALLBACK(file_open), ctx);
    g_signal_connect(mi_file_quit, "activate", G_CALLBACK(gtk_main_quit), NULL);

    // accelerators
    accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(mainwin), accel);
    gtk_widget_add_accelerator(mi_file_new, "activate", accel, GDK_KEY_N, GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(mi_file_open, "activate", accel, GDK_KEY_O, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(mi_file_quit, "activate", accel, GDK_KEY_Q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    return menubar;
}

static void file_open(GtkWidget *w, gpointer data) {
    ExpContext *ctx = data;
    GtkWidget *dlg;
    gchar *xpfile;
    int z;

    dlg = gtk_file_chooser_dialog_new("Open Expense File", GTK_WINDOW(ctx->mainwin),
                                      GTK_FILE_CHOOSER_ACTION_OPEN,
                                      "Open", GTK_RESPONSE_ACCEPT,
                                      "Cancel", GTK_RESPONSE_CANCEL,
                                      NULL);
    z = gtk_dialog_run(GTK_DIALOG(dlg));
    if (z != GTK_RESPONSE_ACCEPT)
        goto exit;

    xpfile = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    if (xpfile == NULL)
        goto exit;
    z = open_expense_file(ctx, xpfile);
    if (z != 0)
        print_error("Error reading expense file");

exit:
    if (xpfile != NULL)
        g_free(xpfile);
    gtk_widget_destroy(dlg);
}

static int open_expense_file(ExpContext *ctx, char *xpfile) {
    int z;

    free_expense_array(ctx->all_xps, countof(ctx->all_xps));

    z = load_expense_file(xpfile, ctx->all_xps, countof(ctx->all_xps), &ctx->all_xps_len);
    if (z != 0)
        return z;
    bs_free(ctx->xpfile);
    ctx->xpfile = strdup(xpfile);

    sort_expenses_by_date_desc(ctx->all_xps, ctx->all_xps_len);
    get_expenses_years(ctx->all_xps, ctx->all_xps_len, ctx->xps_years, countof(ctx->xps_years));
    get_expenses_months(ctx->all_xps, ctx->all_xps_len, ctx->xps_months, countof(ctx->xps_months));
    refresh_filter_ui(ctx);

    filter_expenses(ctx->all_xps, ctx->all_xps_len,
                    ctx->view_xps, &ctx->view_xps_len,
                    "", 0, 0);
    refresh_expenses_treeview(GTK_TREE_VIEW(ctx->tv_xps), ctx->view_xps, ctx->view_xps_len, TRUE);

    return 0;
}

