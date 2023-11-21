#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <gtk/gtk.h>

#include "clib.h"
#include "exp.h"
#include "ui.h"

void quit(const char *s);
void print_error(const char *s);
void panic(const char *s);
static void setup_ui(uictx_t *ctx);
static GtkWidget *create_menubar(uictx_t *ctx, GtkWidget *mainwin);
static int open_expense_file(uictx_t *ctx, char *xpfile);

static void file_open(GtkWidget *w, gpointer data);
static void file_new(GtkWidget *w, gpointer data);
static void file_save(GtkWidget *w, gpointer data);
static void file_saveas(GtkWidget *w, gpointer data);
static void expense_add(GtkWidget *w, gpointer data);
static void expense_edit(GtkWidget *w, gpointer data);
static void expense_delete(GtkWidget *w, gpointer data);

static void add_accel(GtkWidget *w, GtkAccelGroup *a, guint key, GdkModifierType mods);

int main(int argc, char *argv[]) {
    uictx_t *ctx = uictx_new();
    int z;

    gtk_init(&argc, &argv);
    setup_ui(ctx);
//    set_screen_css("app.css");

    if (argc > 1) {
        z = open_expense_file(ctx, argv[1]);
        if (z != 0)
            print_error("Error reading expense file");
    }

    gtk_main();
    uictx_free(ctx);
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

static void setup_ui(uictx_t *ctx) {
    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *statusbar;

    GtkWidget *expenses_sw;
    GtkWidget *expenses_view;
    GtkWidget *expenses_frame;
    GtkWidget *sidebar;
    GtkWidget *hbox1;
    GtkWidget *vbox1;

    GtkWidget *main_vbox;

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

    expenses_frame = create_expenses_section(ctx);
    sidebar = create_sidebar_controls(ctx);
    //g_object_set(sidebar, "margin-top", 5, NULL);
    gtk_widget_set_size_request(sidebar, 100, -1);

    hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(hbox1), sidebar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), expenses_frame, TRUE, TRUE, 0);

    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    g_object_set(vbox1, "margin-start", 10, "margin-end", 10, "margin-top", 10, NULL);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);

    // Main window
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), vbox1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), statusbar, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(mainwin), main_vbox);
    gtk_widget_show_all(mainwin);

    g_signal_connect(mainwin, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_grab_focus(ctx->expenses_view);

    ctx->mainwin = mainwin;
}

static GtkWidget *create_menubar(uictx_t *ctx, GtkWidget *mainwin) {
    GtkWidget *mb;
    GtkWidget *m;
    GtkWidget *mi_file, *mi_file_new, *mi_file_open, *mi_file_save, *mi_file_saveas, *mi_file_quit;
    GtkWidget *mi_expense, *mi_expense_add, *mi_expense_edit, *mi_expense_delete;
    GtkAccelGroup *a;

    mi_file = gtk_menu_item_new_with_mnemonic("_File");
    mi_file_new = gtk_menu_item_new_with_mnemonic("_New");
    mi_file_open = gtk_menu_item_new_with_mnemonic("_Open");
    mi_file_save = gtk_menu_item_new_with_mnemonic("_Save");
    mi_file_saveas = gtk_menu_item_new_with_mnemonic("Save _As");
    mi_file_quit = gtk_menu_item_new_with_mnemonic("_Quit");

    mi_expense = gtk_menu_item_new_with_mnemonic("_Expense");
    mi_expense_add = gtk_menu_item_new_with_mnemonic("_Add");
    mi_expense_edit = gtk_menu_item_new_with_mnemonic("_Edit");
    mi_expense_delete = gtk_menu_item_new_with_mnemonic("_Delete");

    m = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_file_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_file_open);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_file_save);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_file_saveas);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_file_quit);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_file), m);

    m = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_expense_add);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_expense_edit);
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_expense_delete);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_expense), m);

    mb = gtk_menu_bar_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(mb), mi_file);
    gtk_menu_shell_append(GTK_MENU_SHELL(mb), mi_expense);

    // accelerators
    a = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(mainwin), a);
    add_accel(mi_file_new, a, GDK_KEY_N, GDK_CONTROL_MASK);   
    add_accel(mi_file_open, a, GDK_KEY_O, GDK_CONTROL_MASK);   
    add_accel(mi_file_save, a, GDK_KEY_S, GDK_CONTROL_MASK);   
    add_accel(mi_file_saveas, a, GDK_KEY_S, GDK_SHIFT_MASK | GDK_CONTROL_MASK);   
    add_accel(mi_file_quit, a, GDK_KEY_Q, GDK_CONTROL_MASK);   

    add_accel(mi_expense_add, a, GDK_KEY_A, GDK_CONTROL_MASK);
    add_accel(mi_expense_edit, a, GDK_KEY_E, GDK_CONTROL_MASK);
    add_accel(mi_expense_delete, a, GDK_KEY_X, GDK_CONTROL_MASK);

    g_signal_connect(mi_file_new, "activate", G_CALLBACK(file_new), ctx);
    g_signal_connect(mi_file_open, "activate", G_CALLBACK(file_open), ctx);
    g_signal_connect(mi_file_save, "activate", G_CALLBACK(file_save), ctx);
    g_signal_connect(mi_file_saveas, "activate", G_CALLBACK(file_saveas), ctx);

    g_signal_connect(mi_expense_add, "activate", G_CALLBACK(expense_add), ctx);
    g_signal_connect(mi_expense_edit, "activate", G_CALLBACK(expense_edit), ctx);
    g_signal_connect(mi_expense_delete, "activate", G_CALLBACK(expense_delete), ctx);

    g_signal_connect(mi_file_quit, "activate", G_CALLBACK(gtk_main_quit), NULL);

    return mb;
}
static void add_accel(GtkWidget *w, GtkAccelGroup *a, guint key, GdkModifierType mods) {
    gtk_widget_add_accelerator(w, "activate", a, key, mods, GTK_ACCEL_VISIBLE);
}

static void file_open(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    GtkWidget *dlg;
    gchar *xpfile = NULL;
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

static void file_new(GtkWidget *w, gpointer data) {
}
static void file_save(GtkWidget *w, gpointer data) {
}
static void file_saveas(GtkWidget *w, gpointer data) {
}

static void expense_add(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    add_expense_row(ctx);
}
static void expense_edit(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    GtkTreeView *tv = GTK_TREE_VIEW(ctx->expenses_view);
    GtkTreeSelection *ts;
    GtkTreeModel *m;
    GtkTreeIter it;

    ts = gtk_tree_view_get_selection(tv);
    if (!gtk_tree_selection_get_selected(ts, &m, &it))
        return;
    edit_expense_row(tv, &it, ctx);
}
static void expense_delete(GtkWidget *w, gpointer data) {
}


static int open_expense_file(uictx_t *ctx, char *xpfile) {
    FILE *f;

    f = fopen(xpfile, "r");
    if (f == NULL)
        return 1;

    uictx_reset(ctx);

    load_expense_file(ctx, f);
    str_assign(ctx->xpfile, xpfile);
    fclose(f);

    //sort_expenses_by_date_desc(ctx->all_xps, ctx->all_xps_count);

    filter_expenses(ctx);
    refresh_expenses_treeview(GTK_TREE_VIEW(ctx->expenses_view), &ctx->view_xps, TRUE);

    return 0;
}

