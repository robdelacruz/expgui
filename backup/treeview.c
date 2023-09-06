#include <gtk/gtk.h>

void panic_err(GError *err);

static void setup_treeview(GtkWidget *tv);
static void tv_row_activated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer user_data);

int main(int argc, char *argv[]) {
    GError *err = NULL;
    GtkWidget *w;
    GtkWidget *vbox;
    GtkWidget *menubar;
    GtkWidget *filemenu, *filemi, *newmi, *openmi, *quitmi;
    GtkAccelGroup *accel;
    GtkWidget *sw;
    GtkWidget *tv;
    GtkListStore *ls;
    GtkTreeStore *ts;
    GtkTreeIter it, child;

    gtk_init(&argc, &argv);

    // window
    w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(w), "gtktest");
    gtk_window_set_default_size(GTK_WINDOW(w), 640, 300);
    g_signal_connect(w, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // menubar
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

    // treeview
    tv = gtk_tree_view_new();
    setup_treeview(tv);
    g_signal_connect(tv, "row-activated", G_CALLBACK(tv_row_activated), NULL);

    // list_store
    ls = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-16", 1, "globe july", 2, "utilities", 3, 3000.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-16", 1, "subway", 2, "dine_out", 3, 1505.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-18", 1, "andok's", 2, "dine_out", 3, 720.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-20", 1, "MM cat food", 2, "pet_food", 3, 1043.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-22", 1, "coffee bean", 2, "coffee", 3, 630.0, -1);

    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-16", 1, "globe july", 2, "utilities", 3, 3000.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-16", 1, "subway", 2, "dine_out", 3, 1505.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-18", 1, "andok's", 2, "dine_out", 3, 720.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-20", 1, "MM cat food", 2, "pet_food", 3, 1043.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-22", 1, "coffee bean", 2, "coffee", 3, 630.0, -1);

    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-16", 1, "globe july", 2, "utilities", 3, 3000.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-16", 1, "subway", 2, "dine_out", 3, 1505.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-18", 1, "andok's", 2, "dine_out", 3, 720.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-20", 1, "MM cat food", 2, "pet_food", 3, 1043.0, -1);
    gtk_list_store_append(ls, &it);
    gtk_list_store_set(ls, &it, 0, "2023-07-22", 1, "coffee bean", 2, "coffee", 3, 630.0, -1);

    gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ls));
    g_object_unref(ls);

#if 0
    // tree_store
    ts = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT);
    gtk_tree_store_append(ts, &it, NULL);
    gtk_tree_store_set(ts, &it, 0, "utilities", -1);
    gtk_tree_store_append(ts, &child, &it);
    gtk_tree_store_set(ts, &child, 0, "2023-07-16", 1, "globe july", 2, "utilities", 3, 3000.0, -1);

    gtk_tree_store_append(ts, &it, NULL);
    gtk_tree_store_set(ts, &it, 0, "dine_out", -1);

    gtk_tree_store_append(ts, &it, NULL);
    gtk_tree_store_set(ts, &it, 0, "pet_food", -1);

    gtk_tree_store_append(ts, &it, NULL);
    gtk_tree_store_set(ts, &it, 0, "coffee", -1);

    gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ts));
    g_object_unref(ts);
#endif

    // sw: tv
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sw), tv);

    // vbox: menubar, sw
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(w), vbox);
    //gtk_widget_show(w);
    gtk_widget_show_all(w);
    gtk_main();
    return 0;
}

void panic_err(GError *err) {
    fprintf(stderr, "panic: %s\n", err->message);
    g_error_free(err);
    exit(1);
}

static void setup_treeview(GtkWidget *tv) {
    GtkCellRenderer *r;
    GtkTreeViewColumn *col;

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Date", r, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Description", r, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Category", r, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Amount", r, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

}

static void tv_row_activated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer user_data) {
    gchar *s = gtk_tree_path_to_string(tp);
    printf("tv_row_activated() treepath: '%s'\n", s);
    g_free(s);
}

