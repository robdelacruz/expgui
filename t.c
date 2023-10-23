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
    arena_t app_arena, scratch;
    ExpContext ctx;
    int z;

    app_arena = new_arena(10 * MB_BYTES);
    scratch = new_arena(0);
    init_context(&ctx, &app_arena, &scratch);

    gtk_init(&argc, &argv);
    setup_ui(&ctx);

    if (argc > 1) {
        z = open_expense_file(&ctx, argv[1]);
        if (z != 0)
            print_error("Error reading expense file");
    }

    gtk_main();
    reset_context(&ctx);
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
    GdkScreen *screen;
    GtkCssProvider *provider;

    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *statusbar;

    //GtkWidget *notebook;
    GtkWidget *expenses_sw;
    GtkWidget *expenses_view;
    GtkWidget *action_group;
    GtkWidget *hbox1;
    GtkWidget *vbox1;

    GtkWidget *main_vbox;
    GtkWidget *filter_box;

    screen = gdk_screen_get_default();
    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "app.css", NULL);
    //gtk_css_provider_load_from_path(provider, "gtk-3.0/gtk.css", NULL);
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

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

    filter_box = create_filter_section(ctx);
    expenses_view = create_expenses_treeview(ctx);
    expenses_sw = create_scroll_window(expenses_view);
    action_group = create_button_group_vertical();
    g_object_set(action_group, "margin-top", 5, NULL);
    gtk_widget_set_size_request(action_group, 100, -1);

    hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    //gtk_box_pack_start(GTK_BOX(hbox1), expenses_sw, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), create_frame("Expenses", expenses_sw, 10, 10), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), action_group, FALSE, FALSE, 0);

    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    g_object_set(vbox1, "margin-start", 10, "margin-end", 10, "margin-top", 10, NULL);
    gtk_box_pack_start(GTK_BOX(vbox1), filter_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);

    //notebook = gtk_notebook_new();
    //gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    //g_object_set(notebook, "margin-start", 10, "margin-end", 10, NULL);
    //gtk_notebook_append_page(GTK_NOTEBOOK(notebook), expenses_sw, gtk_label_new("Expenses"));

    // Main window
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), vbox1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), statusbar, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(mainwin), main_vbox);
    gtk_widget_show_all(mainwin);

    g_signal_connect(mainwin, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_grab_focus(expenses_view);

    ctx->mainwin = mainwin;
    ctx->menubar = menubar;
    //ctx->notebook = notebook;
    ctx->expenses_view = expenses_view;
    ctx->statusbar = statusbar;
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

static int open_expense_file(ExpContext *ctx, char *xpfile) {
    FILE *f;

    f = fopen(xpfile, "r");
    if (f == NULL)
        return 1;

    reset_context(ctx);

    load_expense_file(f, ctx->all_xps, countof(ctx->all_xps), &ctx->all_xps_count, ctx->arena);
    str_assign(&ctx->xpfile, xpfile);
    fclose(f);

    sort_expenses_by_date_desc(ctx->all_xps, ctx->all_xps_count);
    get_expenses_years(ctx->all_xps, ctx->all_xps_count, ctx->expenses_years, countof(ctx->expenses_years));
    ctx->expenses_cats_count = get_expenses_categories(ctx->all_xps, ctx->all_xps_count, ctx->expenses_cats, countof(ctx->expenses_cats));
    sort_cats_asc(ctx->expenses_cats, ctx->expenses_cats_count);

    refresh_filter_ui(ctx);
    filter_expenses(ctx->all_xps, ctx->all_xps_count,
                    ctx->view_xps, &ctx->view_xps_count,
                    "",
                    ctx->view_month, ctx->view_year,
                    "");
    refresh_expenses_treeview(GTK_TREE_VIEW(ctx->expenses_view), ctx->view_xps, ctx->view_xps_count, TRUE);

    char statustxt[255];
    snprintf(statustxt, sizeof(statustxt), "arena: %ld / %ld, scratch: %ld / %ld", ctx->arena->pos, ctx->arena->cap, ctx->scratch->pos, ctx->scratch->cap);
    guint statusid =  gtk_statusbar_get_context_id(GTK_STATUSBAR(ctx->statusbar), "info");
    gtk_statusbar_push(GTK_STATUSBAR(ctx->statusbar), statusid, statustxt);

    return 0;
}

