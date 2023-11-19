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
static void file_new(GtkWidget *w, gpointer data);
static void file_save(GtkWidget *w, gpointer data);
static void file_saveas(GtkWidget *w, gpointer data);
static void expense_edit(GtkWidget *w, gpointer data);
static void expense_delete(GtkWidget *w, gpointer data);

int main(int argc, char *argv[]) {
    arena_t app_arena, scratch;
    ExpContext ctx;
    int z;

    app_arena = new_arena(10 * MB_BYTES);
    scratch = new_arena(0);
    init_context(&ctx, &app_arena, &scratch);

    gtk_init(&argc, &argv);
    setup_ui(&ctx);
//    set_screen_css("app.css");

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
    GtkWidget *filter_box;

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
    ctx->menubar = menubar;
    ctx->statusbar = statusbar;
}

static GtkWidget *create_menubar(ExpContext *ctx, GtkWidget *mainwin) {
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

    g_signal_connect(mi_expense_edit, "activate", G_CALLBACK(expense_edit), ctx);
    g_signal_connect(mi_expense_delete, "activate", G_CALLBACK(expense_delete), ctx);

    g_signal_connect(mi_file_quit, "activate", G_CALLBACK(gtk_main_quit), NULL);

    return mb;
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

static void file_new(GtkWidget *w, gpointer data) {
}
static void file_save(GtkWidget *w, gpointer data) {
}
static void file_saveas(GtkWidget *w, gpointer data) {
}

static void expense_edit(GtkWidget *w, gpointer data) {
    ExpContext *ctx = data;
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

