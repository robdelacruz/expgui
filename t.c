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
static void setupui(ExpContext *ctx);
static GtkWidget *create_menubar(ExpContext *ctx, GtkWidget *mainwin);
static GtkWidget *create_expenses_treeview();
static void amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data);

static void file_open(GtkWidget *w, gpointer data);
static void filter_changed(GtkWidget *txtfilter, gpointer data);

int main(int argc, char *argv[]) {
    ExpContext *ctx;

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

static void setupui(ExpContext *ctx) {
    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *notebook;
    GtkWidget *tv_xps;
    GtkWidget *sw_xps;
    GtkWidget *txt_filter;
    GtkWidget *cb_month;
    GtkWidget *cb_year;
    GtkWidget *vbox1;
    GtkWidget *hbox1;

    // mainwin
    mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainwin), "gtktest");
    gtk_window_set_default_size(GTK_WINDOW(mainwin), 640, 480);
    gtk_container_set_border_width(GTK_CONTAINER(mainwin), 10);
    g_signal_connect(mainwin, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    menubar = create_menubar(ctx, mainwin);

    // Filter section
    hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    txt_filter = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(txt_filter), "Filter Expenses");
    cb_year = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_year), "2016");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_year), "2017");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_year), "2018");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_year), "2019");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_year), "2020");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_year), "2021");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_year), "2022");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_year), "2023");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cb_year), 0);
    cb_month = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_month), "January");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_month), "February");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_month), "March");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cb_month), 0);
    gtk_box_pack_start(GTK_BOX(hbox1), txt_filter, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), cb_year, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), cb_month, FALSE, FALSE, 0);

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);

    // Expenses list
    sw_xps = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_xps), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    tv_xps = create_expenses_treeview();
    gtk_container_add(GTK_CONTAINER(sw_xps), tv_xps);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw_xps, gtk_label_new("Expenses"));

    // Main window
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox1), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), notebook, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(mainwin), vbox1);
    gtk_widget_show_all(mainwin);

    g_signal_connect(txt_filter, "changed", G_CALLBACK(filter_changed), ctx);

    gtk_widget_grab_focus(tv_xps);

    ctx->mainwin = mainwin;
    ctx->menubar = menubar;
    ctx->notebook = notebook;
    ctx->tv_xps = tv_xps;
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

static void file_open(GtkWidget *w, gpointer data) {
    ExpContext *ctx = data;
    GtkWidget *dlg;
    gchar *xpfile;
    gint z;

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

    printf("Opening '%s'\n", xpfile);
    z = load_expense_file(ctx, xpfile);
    if (z != 0)
        print_error("Error reading expense file");
    g_free(xpfile);

exit:
    gtk_widget_destroy(dlg);
}

static void filter_changed(GtkWidget *txtfilter, gpointer data) {
    const gchar *sfilter = gtk_entry_get_text(GTK_ENTRY(txtfilter));
    printf("sfilter: '%s'\n", sfilter);
}

