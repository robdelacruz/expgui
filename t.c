#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <gtk/gtk.h>

#include "bslib.h"
#include "expense.h"

static inline void quit(const char *s);
static inline void print_error(const char *s);
static inline void panic(const char *s);
static void setupui();
GtkWidget *create_menubar(GtkWidget *w);

int main(int argc, char *argv[]) {
    char *expfile = NULL;
    BSArray *exps = NULL;

    if (argc > 1) {
        expfile = argv[1];
        exps = load_expense_file(expfile);
        if (exps == NULL)
            panic("Error reading expense file");
    } else {
        exps = bs_array_type_new(Expense, 20);
    }

    gtk_init(&argc, &argv);
    setupui();
    gtk_main();

    return 0;
}

static inline void quit(const char *s) {
    if (s)
        printf("%s\n", s);
    exit(0);
}
static inline void print_error(const char *s) {
    if (s)
        fprintf(stderr, "%s: %s\n", s, strerror(errno));
    else
        fprintf(stderr, "%s\n", strerror(errno));
}
static inline void panic(const char *s) {
    print_error(s);
    exit(1);
}

static void setupui() {
    GtkWidget *w;
    GtkWidget *vbox;
    GtkWidget *mb;

    GtkWidget *nb;
    GtkWidget *nb_lbl1, *nb_lbl2, *nb_lbl3;
    GtkWidget *expenses, *catsum, *ytd;

    // window
    w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(w), "gtktest");
    gtk_window_set_default_size(GTK_WINDOW(w), 640, 300);
    gtk_container_set_border_width(GTK_CONTAINER(w), 10);
    g_signal_connect(w, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    mb = create_menubar(w);

    // notebook
    nb = gtk_notebook_new();
    nb_lbl1 = gtk_label_new("Expenses");
    nb_lbl2 = gtk_label_new("Category Summary");
    nb_lbl3 = gtk_label_new("Yearly Summary");

    expenses = gtk_label_new("Expenses list");
    catsum = gtk_label_new("Category Summary");
    ytd = gtk_label_new("Year to date");

    gtk_notebook_append_page(GTK_NOTEBOOK(nb), expenses, nb_lbl1);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), catsum, nb_lbl2);
    gtk_notebook_append_page(GTK_NOTEBOOK(nb), ytd, nb_lbl3);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(nb), GTK_POS_BOTTOM);

    // vbox: menubar, nb
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), mb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), nb, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(w), vbox);
    gtk_widget_show_all(w);
}

GtkWidget *create_menubar(GtkWidget *w) {
    GtkWidget *mb;
    GtkWidget *filemenu, *filemi, *newmi, *openmi, *quitmi;
    GtkAccelGroup *accel;

    mb = gtk_menu_bar_new();
    filemenu = gtk_menu_new();
    filemi = gtk_menu_item_new_with_mnemonic("_File");
    newmi = gtk_menu_item_new_with_mnemonic("_New");
    openmi = gtk_menu_item_new_with_mnemonic("_Open");
    quitmi = gtk_menu_item_new_with_mnemonic("_Quit");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(filemi), filemenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), newmi);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), openmi);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), quitmi);
    gtk_menu_shell_append(GTK_MENU_SHELL(mb), filemi);
    g_signal_connect(quitmi, "activate", G_CALLBACK(gtk_main_quit), NULL);

    // accelerators
    accel = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(w), accel);
    gtk_widget_add_accelerator(newmi, "activate", accel, GDK_KEY_N, GDK_SHIFT_MASK | GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(openmi, "activate", accel, GDK_KEY_O, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(quitmi, "activate", accel, GDK_KEY_Q, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    return mb;
}

