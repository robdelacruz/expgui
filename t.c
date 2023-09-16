#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <gtk/gtk.h>

#include "bslib.h"
#include "expense.h"

void quit(const char *s);
void print_error(const char *s);
void panic(const char *s);
static void setupui(Context *ctx);
static GtkWidget *create_menubar(GtkWidget *w);
static GtkWidget *create_expenses_treeview();

static void amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data);

int main(int argc, char *argv[]) {
    Context *ctx;

    ctx = create_context();
    gtk_init(&argc, &argv);
    setupui(ctx);

    if (argc > 1) {
        int z = load_expense_file(ctx, argv[1]);
        if (z != 0)
            panic("Error loading expense file");
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

static void setupui(Context *ctx) {
    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *notebook;
    GtkWidget *tv_xps;
    GtkWidget *sw_expenses;
    GtkWidget *vbox;

    // window
    mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainwin), "gtktest");
    gtk_window_set_default_size(GTK_WINDOW(mainwin), 640, 300);
    gtk_container_set_border_width(GTK_CONTAINER(mainwin), 10);
    g_signal_connect(mainwin, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    menubar = create_menubar(mainwin);

    sw_expenses = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_expenses), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    tv_xps = create_expenses_treeview();
    gtk_container_add(GTK_CONTAINER(sw_expenses), tv_xps);

    // notebook: sw_expenses
    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw_expenses, gtk_label_new("Expenses"));

    // vbox: menubar, notebook
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(mainwin), vbox);
    gtk_widget_show_all(mainwin);

    ctx->mainwin = mainwin;
    ctx->menubar = menubar;
    ctx->notebook = notebook;
    ctx->tv_xps = tv_xps;
}

static GtkWidget *create_menubar(GtkWidget *w) {
    GtkWidget *menubar;
    GtkWidget *filemenu, *filemi, *newmi, *openmi, *quitmi;
    GtkAccelGroup *accel;

    menubar = gtk_menu_bar_new();
    filemenu = gtk_menu_new();
    filemi = gtk_menu_item_new_with_mnemonic("_File");
    newmi = gtk_menu_item_new_with_mnemonic("_New");
    openmi = gtk_menu_item_new_with_mnemonic("_Open");
    quitmi = gtk_menu_item_new_with_mnemonic("_Quit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(filemi), filemenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), newmi);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), openmi);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), quitmi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), filemi);
    g_signal_connect(quitmi, "activate", G_CALLBACK(gtk_main_quit), NULL);

    // accelerators
    accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(w), accel);
    gtk_widget_add_accelerator(newmi, "activate", accel, GDK_KEY_N, GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(openmi, "activate", accel, GDK_KEY_O, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(quitmi, "activate", accel, GDK_KEY_Q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    return menubar;
}

static GtkWidget *create_expenses_treeview() {
    GtkWidget *tv;
    GtkCellRenderer *r;
    GtkTreeViewColumn *col;
    GtkListStore *ls;

    tv = gtk_tree_view_new();

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Date", r, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Description", r, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Amount", r, "text", 2, NULL);
    gtk_tree_view_column_set_cell_data_func(col, r, amt_datafunc, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Category", r, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    ls = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ls));
    g_object_unref(ls);

    return tv;
}

static void amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data) {
    float amt;
    char buf[15];

    gtk_tree_model_get(m, it, 2, &amt, -1);
    snprintf(buf, sizeof(buf), "%9.2f", amt);
    g_object_set(r, "text", buf, NULL);
}

