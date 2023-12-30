#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <locale.h>

#include "ui.h"

typedef struct {
    GtkDialog *dlg;
    GtkEntry *txt_date;
    GtkCalendar *cal;
    GtkEntry *txt_desc;
    GtkEntry *txt_amt;
    GtkComboBox *cbcat;
    int showcal;
} ExpenseEditDialog;

static char *_month_names[] = {"All", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

static void set_screen_css(char *cssfile);
static GtkWidget *create_scroll_window(GtkWidget *child);
static GtkWidget *create_frame(gchar *label, GtkWidget *child, int xpadding, int ypadding);
static void add_accel(GtkWidget *w, GtkAccelGroup *a, guint key, GdkModifierType mods);
static void cancel_wait_id(guint *wait_id);
static void remove_children(GtkWidget *w);

static gboolean tv_get_selected_it(GtkTreeView *tv, GtkTreeIter *it);
static gboolean tv_get_prev_it(GtkTreeView *tv, GtkTreeIter *it);
static gboolean tv_get_next_it(GtkTreeView *tv, GtkTreeIter *it);
static void tv_set_cursor(GtkTreeView *tv, GtkTreeIter *it);

static gboolean mainwin_keypress(GtkTreeView *tv, GdkEventKey *e, gpointer data);

// Main menu bar
static GtkWidget *mainmenu_new(uictx_t *ctx, GtkWidget *mainwin);
static void mainmenu_file_open(GtkWidget *w, gpointer data);
static void mainmenu_file_new(GtkWidget *w, gpointer data);
static void mainmenu_file_save(GtkWidget *w, gpointer data);
static void mainmenu_file_saveas(GtkWidget *w, gpointer data);
static void mainmenu_expense_add(GtkWidget *w, gpointer data);
static void mainmenu_expense_edit(GtkWidget *w, gpointer data);
static void mainmenu_expense_delete(GtkWidget *w, gpointer data);
static void mainmenu_setup_cat(GtkWidget *w, gpointer data);

// Expenses Treeview
GtkWidget *tv_new(uictx_t *ctx);
static void tv_amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data);
static void tv_date_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data);
static void tv_row_activated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer data);
static void tv_row_changed(GtkTreeSelection *ts, gpointer data);
static gboolean tv_keypress(GtkTreeView *tv, GdkEventKey *e, gpointer data);
static gboolean txt_filter_keypress(GtkTreeView *tv, GdkEventKey *e, gpointer data);

static void tv_get_expense(GtkTreeModel *m, GtkTreeIter *it, exp_t *xp);
static void tv_set_expense(GtkListStore *m, GtkTreeIter *it, db_t *db, exp_t *xp);
static void tv_refresh(GtkTreeView *tv, uictx_t *ctx, gboolean reset_cursor);
static void tv_set_cursor_to_rowid(GtkTreeView *tv, uint rowid);
static void tv_filter_changed(GtkWidget *w, gpointer data);
static gboolean tv_apply_filter(gpointer data);

static void tv_add_expense_row(uictx_t *ctx);
static void tv_edit_expense_row(GtkTreeView *tv, GtkTreeIter *it, uictx_t *ctx);
static void tv_del_expense_row(GtkTreeView *tv, GtkTreeIter *it, uictx_t *ctx);

// Sidebar
static GtkWidget *sidebar_new(uictx_t *ctx);
static void sidebar_refresh(uictx_t *ctx);
static void sidebar_populate_year_menu(GtkWidget *menu, uictx_t *ctx);
static void sidebar_populate_month_menu(GtkWidget *menu, uictx_t *ctx);
static void yearmenu_select(GtkWidget *w, gpointer data);
static void monthmenu_select(GtkWidget *w, gpointer data);

static int cat_row_from_id(db_t *db, uint catid);
static uint cat_id_from_row(db_t *db, int row);

static ExpenseEditDialog *expeditdlg_new(db_t *db, exp_t *xp);
static void expeditdlg_free(ExpenseEditDialog *d);
static void expeditdlg_get_expense(ExpenseEditDialog *d, db_t *db, exp_t *xp);
static void expeditdlg_amt_insert_text_event(GtkEntry* ed, gchar *new_txt, gint len, gint *pos, gpointer data);
static void expeditdlg_date_icon_press_event(GtkEntry *ed, GtkEntryIconPosition pos, GdkEvent *e, gpointer data);
static void expeditdlg_date_insert_text_event(GtkEntry* ed, gchar *newtxt, gint newtxt_len, gint *pos, gpointer data);
static void expeditdlg_date_delete_text_event(GtkEntry* ed, gint startpos, gint endpos, gpointer data);
static gboolean expeditdlg_date_key_press_event(GtkEntry *ed, GdkEventKey *e, gpointer data);
static void expeditdlg_cal_day_selected_event(GtkCalendar *cal, gpointer data);
static void expeditdlg_cal_day_selected_dblclick_event(GtkCalendar *cal, gpointer data);

static GtkWidget *expdeldlg_new(db_t *db, exp_t *xp);
static GtkWidget *cateditdlg_new(db_t *db);
static void cateditdlg_name_edited(GtkCellRendererText *r, gchar *path, gchar *newname, GtkTreeView *tv);
static void cateditdlg_row_activated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer data);
static void cateditdlg_add_clicked(GtkWidget *w, GtkTreeView *tv);

static void set_screen_css(char *cssfile) {
    GdkScreen *screen = gdk_screen_get_default();
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, cssfile, NULL);
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static GtkWidget *create_scroll_window(GtkWidget *child) {
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(sw), child);

    return sw;
}

static GtkWidget *create_frame(gchar *label, GtkWidget *child, int xpadding, int ypadding) {
    GtkWidget *fr = gtk_frame_new(label);
    if (xpadding > 0)
        g_object_set(child, "margin-start", xpadding, "margin-end", xpadding, NULL);
    if (ypadding > 0)
        g_object_set(child, "margin-top", xpadding, "margin-bottom", xpadding, NULL);
    gtk_container_add(GTK_CONTAINER(fr), child);
    return fr;
}

static void add_accel(GtkWidget *w, GtkAccelGroup *a, guint key, GdkModifierType mods) {
    gtk_widget_add_accelerator(w, "activate", a, key, mods, GTK_ACCEL_VISIBLE);
}

static void cancel_wait_id(guint *wait_id) {
    if (*wait_id != 0) {
        g_source_remove(*wait_id);
        *wait_id = 0;
    }
}

static void remove_child_func(gpointer data, gpointer user_data) {
    GtkWidget *child = GTK_WIDGET(data);
    GtkContainer *parent = GTK_CONTAINER(user_data);
    gtk_container_remove(parent, child);
}
static void remove_children(GtkWidget *w) {
    g_list_foreach(gtk_container_get_children(GTK_CONTAINER(w)), remove_child_func, w);
}

static gboolean tv_get_selected_it(GtkTreeView *tv, GtkTreeIter *it) {
    GtkTreeSelection *ts = gtk_tree_view_get_selection(tv);
    return gtk_tree_selection_get_selected(ts, NULL, it);
}
static gboolean tv_get_prev_it(GtkTreeView *tv, GtkTreeIter *it) {
    GtkTreeModel *m = gtk_tree_view_get_model(tv);
    return gtk_tree_model_iter_previous(m, it);
}
static gboolean tv_get_next_it(GtkTreeView *tv, GtkTreeIter *it) {
    GtkTreeModel *m = gtk_tree_view_get_model(tv);
    return gtk_tree_model_iter_next(m, it);
}
static void tv_set_cursor(GtkTreeView *tv, GtkTreeIter *it) {
    GtkTreeModel *m = gtk_tree_view_get_model(tv);
    GtkTreePath *tp = gtk_tree_model_get_path(m, it);
    gtk_tree_view_set_cursor(tv, tp, NULL, FALSE);
}

uictx_t *uictx_new() {
    uictx_t *ctx = malloc(sizeof(uictx_t));

    ctx->xpfile = str_new(0);
    ctx->db = db_new();

    ctx->view_filter = str_new(0);
    ctx->view_year = 0;
    ctx->view_month = 0;
    ctx->view_wait_id = 0;

    ctx->mainwin = NULL;
    ctx->expenses_view = NULL;
    ctx->txt_filter = NULL;
    ctx->yearmenu = NULL;
    ctx->yearbtn = NULL;
    ctx->monthbtn = NULL;
    ctx->yearbtnlabel = NULL;
    ctx->monthbtnlabel = NULL;

    return ctx;
}
void uictx_free(uictx_t *ctx) {
    str_free(ctx->xpfile);
    str_free(ctx->view_filter);
    db_free(ctx->db);
    free(ctx);
}
void uictx_reset(uictx_t *ctx) {
    str_assign(ctx->xpfile, "");
    str_assign(ctx->view_filter, "");
    ctx->view_year = 0;
    ctx->view_month = 0;
    ctx->view_wait_id = 0;
    db_reset(ctx->db);
}

void setupui(uictx_t *ctx) {
    GtkWidget *mainwin;
    GtkWidget *menubar;

    GtkWidget *expenses_sw;
    GtkWidget *expenses_view;
    GtkWidget *expenses_frame;
    GtkWidget *sidebar;
    GtkWidget *main_vbox;

    setlocale(LC_NUMERIC, "");

    // mainwin
    mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainwin), "Expense Buddy");
    //gtk_widget_set_size_request(mainwin, 800, 600);
    gtk_window_set_default_size(GTK_WINDOW(mainwin), 480, 480);

    menubar = mainmenu_new(ctx, mainwin);
    expenses_frame = tv_new(ctx);
    sidebar = sidebar_new(ctx);

    // Main window
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), expenses_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), sidebar, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(mainwin), main_vbox);
    gtk_widget_show_all(mainwin);

    g_signal_connect(mainwin, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(mainwin, "key-press-event", G_CALLBACK(mainwin_keypress), ctx);

    gtk_widget_grab_focus(ctx->expenses_view);

    ctx->mainwin = mainwin;
}
static gboolean mainwin_keypress(GtkTreeView *tv, GdkEventKey *e, gpointer data) {
    uictx_t *ctx = data;
    uint kv = e->keyval;

    if (e->state == GDK_CONTROL_MASK && kv == GDK_KEY_k) {
        gtk_widget_grab_focus(ctx->txt_filter);
        return TRUE;
    } else if (e->state == GDK_CONTROL_MASK && kv == GDK_KEY_g) {
        gtk_widget_grab_focus(ctx->expenses_view);
        return TRUE;
    }
    return FALSE;
}


int open_expense_file(uictx_t *ctx, char *xpfile) {
    FILE *f;
    char *filter;
    db_t *db = ctx->db;

    f = fopen(xpfile, "r");
    if (f == NULL)
        return 1;

    uictx_reset(ctx);
    db_load_expense_file(db, f);
    str_assign(ctx->xpfile, xpfile);
    fclose(f);

    str_assign(ctx->view_filter, "");

    // Initialize view year/month to most recent year/month.
    if (db->xps->len > 0) {
        exp_t *xp = db->xps->items[0];
        ctx->view_year = date_year(xp->dt);
        ctx->view_month = date_month(xp->dt);
    } else {
        ctx->view_year = 0;
        ctx->view_month = 0;
    }

    tv_refresh(GTK_TREE_VIEW(ctx->expenses_view), ctx, TRUE);
    sidebar_refresh(ctx);

    return 0;
}

static GtkWidget *mainmenu_new(uictx_t *ctx, GtkWidget *mainwin) {
    GtkWidget *mb;
    GtkWidget *m;
    GtkWidget *mi_file, *mi_file_new, *mi_file_open, *mi_file_save, *mi_file_saveas, *mi_file_quit;
    GtkWidget *mi_expense, *mi_expense_add, *mi_expense_edit, *mi_expense_delete;
    GtkWidget *mi_setup, *mi_setup_cat;
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

    mi_setup = gtk_menu_item_new_with_mnemonic("_Setup");
    mi_setup_cat = gtk_menu_item_new_with_mnemonic("_Categories");

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

    m = gtk_menu_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(m), mi_setup_cat);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi_setup), m);

    mb = gtk_menu_bar_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(mb), mi_file);
    gtk_menu_shell_append(GTK_MENU_SHELL(mb), mi_expense);
    gtk_menu_shell_append(GTK_MENU_SHELL(mb), mi_setup);

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

    add_accel(mi_setup_cat, a, GDK_KEY_C, GDK_CONTROL_MASK);

    g_signal_connect(mi_file_new, "activate", G_CALLBACK(mainmenu_file_new), ctx);
    g_signal_connect(mi_file_open, "activate", G_CALLBACK(mainmenu_file_open), ctx);
    g_signal_connect(mi_file_save, "activate", G_CALLBACK(mainmenu_file_save), ctx);
    g_signal_connect(mi_file_saveas, "activate", G_CALLBACK(mainmenu_file_saveas), ctx);
    g_signal_connect(mi_file_quit, "activate", G_CALLBACK(gtk_main_quit), NULL);

    g_signal_connect(mi_expense_add, "activate", G_CALLBACK(mainmenu_expense_add), ctx);
    g_signal_connect(mi_expense_edit, "activate", G_CALLBACK(mainmenu_expense_edit), ctx);
    g_signal_connect(mi_expense_delete, "activate", G_CALLBACK(mainmenu_expense_delete), ctx);

    g_signal_connect(mi_setup_cat, "activate", G_CALLBACK(mainmenu_setup_cat), ctx);


    return mb;
}

static void mainmenu_file_open(GtkWidget *w, gpointer data) {
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
static void mainmenu_file_new(GtkWidget *w, gpointer data) {
}
static void mainmenu_file_save(GtkWidget *w, gpointer data) {
    FILE *f;
    uictx_t *ctx = data;

    if (ctx->xpfile->len == 0) {
        fprintf(stderr, "No expense file\n");
        return;
    }
    f = fopen(ctx->xpfile->s, "w");
    if (f == NULL) {
        print_error("Error opening expense file");
        return;
    }

    db_save_expense_file(ctx->db, f);
    fclose(f);
    fprintf(stderr, "Expense file '%s' saved.\n", ctx->xpfile->s);
}
static void mainmenu_file_saveas(GtkWidget *w, gpointer data) {
}
static void mainmenu_expense_add(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    tv_add_expense_row(ctx);
}
static void mainmenu_expense_edit(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    GtkTreeView *tv = GTK_TREE_VIEW(ctx->expenses_view);
    GtkTreeSelection *ts;
    GtkTreeModel *m;
    GtkTreeIter it;

    ts = gtk_tree_view_get_selection(tv);
    if (!gtk_tree_selection_get_selected(ts, &m, &it))
        return;
    tv_edit_expense_row(tv, &it, ctx);
}
static void mainmenu_expense_delete(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    GtkTreeView *tv = GTK_TREE_VIEW(ctx->expenses_view);
    GtkTreeSelection *ts;
    GtkTreeModel *m;
    GtkTreeIter it;

    ts = gtk_tree_view_get_selection(tv);
    if (!gtk_tree_selection_get_selected(ts, &m, &it))
        return;
    tv_del_expense_row(tv, &it, ctx);
}

static void mainmenu_setup_cat(GtkWidget *w, gpointer data) {
    GtkWidget *dlg;
    uictx_t *ctx = data;
    db_t *db = ctx->db;

    dlg = cateditdlg_new(db);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

enum exp_field_t {
    EXP_FIELD_ROWID,
    EXP_FIELD_DATE,
    EXP_FIELD_DESC,
    EXP_FIELD_AMT,
    EXP_FIELD_CATID,
    EXP_FIELD_CATNAME,
    EXP_FIELD_NCOLS
};

enum cat_field_t {
    CAT_FIELD_ID,
    CAT_FIELD_NAME,
    CAT_FIELD_NCOLS
};

GtkWidget *tv_new(uictx_t *ctx) {
    GtkWidget *frame;
    GtkWidget *vbox;
    GtkWidget *tv;
    GtkWidget *sw;
    GtkTreeSelection *ts;
    GtkCellRenderer *r;
    GtkTreeViewColumn *col;
    GtkListStore *ls;
    GtkWidget *txt_filter;
    int xpadding = 10;
    int ypadding = 2;

    tv = gtk_tree_view_new();
    sw = create_scroll_window(tv);
    g_object_set(tv, "enable-search", FALSE, NULL);
    gtk_widget_add_events(tv, GDK_KEY_PRESS_MASK);

    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    gtk_tree_selection_set_mode(ts, GTK_SELECTION_BROWSE);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Date", r, "text", EXP_FIELD_DATE, NULL);
    gtk_tree_view_column_set_sort_column_id(col, EXP_FIELD_DATE);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_column_set_cell_data_func(col, r, tv_date_datafunc, ctx, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Description", r, "text", EXP_FIELD_DESC, NULL);
    gtk_tree_view_column_set_sort_column_id(col, EXP_FIELD_DESC);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(col, 200);
    gtk_tree_view_column_set_expand(col, FALSE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Amount", r, "text", EXP_FIELD_AMT, NULL);
    gtk_tree_view_column_set_sort_column_id(col, EXP_FIELD_AMT);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_cell_renderer_set_alignment(r, 1.0, 0);
    gtk_tree_view_column_set_cell_data_func(col, r, tv_amt_datafunc, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Category", r, "text", EXP_FIELD_CATNAME, NULL);
    gtk_tree_view_column_set_sort_column_id(col, EXP_FIELD_CATNAME);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    ls = gtk_list_store_new(EXP_FIELD_NCOLS,
                            G_TYPE_UINT,   // EXP_FIELD_ROWID
                            G_TYPE_LONG,   // EXP_FIELD_DATE
                            G_TYPE_STRING, // EXP_FIELD_DESC
                            G_TYPE_DOUBLE, // EXP_FIELD_AMT
                            G_TYPE_UINT,   // EXP_FIELD_CATID
                            G_TYPE_STRING  // EXP_FIELD_CATNAME
                            );
    gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ls));
    g_object_unref(ls);

    txt_filter = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(txt_filter), "Filter Expenses");

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), txt_filter, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    frame = create_frame("", vbox, 4, 0);

    g_signal_connect(tv, "row-activated", G_CALLBACK(tv_row_activated), ctx);
//    g_signal_connect(ts, "changed", G_CALLBACK(tv_row_changed), ctx);
    g_signal_connect(tv, "key-press-event", G_CALLBACK(tv_keypress), ctx);
    g_signal_connect(txt_filter, "changed", G_CALLBACK(tv_filter_changed), ctx);
    g_signal_connect(txt_filter, "key-press-event", G_CALLBACK(txt_filter_keypress), ctx);

    ctx->expenses_view = tv;
    ctx->txt_filter = txt_filter;
    return frame;
}

static void tv_amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data) {
    gdouble amt;
    char buf[15];

    gtk_tree_model_get(m, it, EXP_FIELD_AMT, &amt, -1);
    snprintf(buf, sizeof(buf), "%'9.2f", amt);
    g_object_set(r, "text", buf, NULL);
}
static void tv_date_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data) {
    uictx_t *ctx = data;
    date_t *dt;
    time_t time;
    char buf[30];
    char *fmt;

    gtk_tree_model_get(m, it, EXP_FIELD_DATE, &time, -1);
    dt = date_new(time);

    if (ctx->view_year == 0)
        fmt = "%Y-%m-%d";
    else if (ctx->view_month == 0)
        fmt = "%b %d";
    else
        fmt = "%a %d";

    date_strftime(dt, fmt, buf, sizeof(buf));
    g_object_set(r, "text", buf, NULL);
    date_free(dt);
}

static void tv_row_activated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer data) {
    uictx_t *ctx = data;
    GtkTreeSelection *ts;
    GtkTreeModel *m;
    GtkTreeIter it;

    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    if (!gtk_tree_selection_get_selected(ts, &m, &it))
        return;
    tv_edit_expense_row(tv, &it, ctx);
}

static void tv_row_changed(GtkTreeSelection *ts, gpointer data) {
    uictx_t *ctx = data;
    GtkTreeView *tv;
    GtkListStore *ls;
    GtkTreeIter it;

    if (!gtk_tree_selection_get_selected(ts, (GtkTreeModel **)&ls, &it))
        return;
    tv = gtk_tree_selection_get_tree_view(ts);
}

static gboolean tv_keypress(GtkTreeView *tv, GdkEventKey *e, gpointer data) {
    uictx_t *ctx = data;
    GtkTreeSelection *ts;
    GtkTreeModel *m;
    GtkTreeIter it;
    GtkTreePath *tp;
    uint kv = e->keyval;

    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    if (!gtk_tree_selection_get_selected(ts, &m, &it))
        return FALSE;

    if (kv == GDK_KEY_k) {
        if (!gtk_tree_model_iter_previous(m, &it))
            return FALSE;
        gtk_tree_selection_select_iter(ts, &it);
        tp = gtk_tree_model_get_path(m, &it);
        gtk_tree_view_set_cursor(tv, tp, NULL, FALSE);
        gtk_tree_path_free(tp);
        return TRUE;
    } else if (kv == GDK_KEY_j) {
        if (!gtk_tree_model_iter_next(m, &it))
            return FALSE;
        gtk_tree_selection_select_iter(ts, &it);
        tp = gtk_tree_model_get_path(m, &it);
        gtk_tree_view_set_cursor(tv, tp, NULL, FALSE);
        gtk_tree_path_free(tp);
        return TRUE;
    } else if (kv == GDK_KEY_Delete || (e->state == GDK_CONTROL_MASK && kv == GDK_KEY_x)) {
        tv_del_expense_row(tv, &it, ctx);
        return TRUE;
    }
    return FALSE;
}

static gboolean txt_filter_keypress(GtkTreeView *tv, GdkEventKey *e, gpointer data) {
    uictx_t *ctx = data;
    uint kv = e->keyval;

    if (kv == GDK_KEY_Escape) {
        gtk_entry_set_text(GTK_ENTRY(ctx->txt_filter), "");
        return TRUE;
    } else if (kv == GDK_KEY_Return) {
        gtk_widget_grab_focus(ctx->expenses_view);
        return TRUE;
    }
    return FALSE;
}

static void tv_get_expense(GtkTreeModel *m, GtkTreeIter *it, exp_t *xp) {
    time_t time;
    gchar *desc;

    gtk_tree_model_get(m, it,
                       EXP_FIELD_ROWID, &xp->rowid,
                       EXP_FIELD_DATE, &time,
                       EXP_FIELD_DESC, &desc,
                       EXP_FIELD_AMT, &xp->amt,
                       EXP_FIELD_CATID, &xp->catid,
                       -1);

    date_assign_time(xp->dt, time);
    str_assign(xp->desc, desc);

    g_free(desc);
}
static void tv_set_expense(GtkListStore *m, GtkTreeIter *it, db_t *db, exp_t *xp) {
    gtk_list_store_set(m, it, 
                       EXP_FIELD_ROWID, xp->rowid,
                       EXP_FIELD_DATE, date_time(xp->dt),
                       EXP_FIELD_DESC, xp->desc->s,
                       EXP_FIELD_AMT, xp->amt,
                       EXP_FIELD_CATID, xp->catid,
                       EXP_FIELD_CATNAME, db_find_cat_name(db, xp->catid),
                       -1);
}

static void tv_refresh(GtkTreeView *tv, uictx_t *ctx, gboolean reset_cursor) {
    GtkListStore *ls;
    GtkTreeIter it;
    GtkTreePath *tp;
    GtkTreeSelection *ts;
    exp_t *xp;
    uint cur_rowid = 0;

    db_t *db = ctx->db;
    str_t *filter = ctx->view_filter;
    uint year = ctx->view_year;
    uint month = ctx->view_month;

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
    assert(ls != NULL);

    // (a) Remember active row to be restored later.
    gtk_tree_view_get_cursor(tv, &tp, NULL);
    if (tp) {
        gtk_tree_model_get_iter(GTK_TREE_MODEL(ls), &it, tp);
        gtk_tree_model_get(GTK_TREE_MODEL(ls), &it, EXP_FIELD_ROWID, &cur_rowid, -1);
    }
    gtk_tree_path_free(tp);

    // Turn off selection while refreshing treeview so we don't get
    // bombarded by 'change' events.
    //ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    //gtk_tree_selection_set_mode(ts, GTK_SELECTION_NONE);

    gtk_list_store_clear(ls);

    for (int i=0; i < db->xps->len; i++) {
        xp = db->xps->items[i];
        if (filter->len != 0 && strcasestr(xp->desc->s, filter->s) == NULL)
            continue;
        if (year != 0 && year != date_year(xp->dt))
            continue;
        if (month != 0 && month != date_month(xp->dt))
            continue;

        gtk_list_store_append(ls, &it);
        tv_set_expense(ls, &it, db, xp);

        // (a) Restore previously active row.
        if (xp->rowid == cur_rowid) {
            tp = gtk_tree_model_get_path(GTK_TREE_MODEL(ls), &it);
            gtk_tree_view_set_cursor(tv, tp, NULL, FALSE);
            gtk_tree_path_free(tp);
        }
    }

    //gtk_tree_selection_set_mode(ts, GTK_SELECTION_BROWSE);

    if (reset_cursor) {
        tp = gtk_tree_path_new_from_string("0");
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(tv), tp, NULL, FALSE);
        gtk_tree_path_free(tp);
    }
}

static void tv_set_cursor_to_rowid(GtkTreeView *tv, uint rowid) {
    GtkTreeIter it;
    GtkTreePath *tp;
    GtkTreeModel *ls = gtk_tree_view_get_model(tv);
    uint itrowid = 0;

    if (!gtk_tree_model_get_iter_first(ls, &it))
        return;
    gtk_tree_model_get(ls, &it, EXP_FIELD_ROWID, &itrowid, -1);
    if (itrowid == rowid) {
        tp = gtk_tree_model_get_path(ls, &it);
        gtk_tree_view_set_cursor(tv, tp, NULL, FALSE);
        return;
    }
    while(gtk_tree_model_iter_next(ls, &it)) {
        gtk_tree_model_get(ls, &it, EXP_FIELD_ROWID, &itrowid, -1);
        if (itrowid == rowid) {
            tp = gtk_tree_model_get_path(ls, &it);
            gtk_tree_view_set_cursor(tv, tp, NULL, FALSE);
            return;
        }
    }
}

static void tv_filter_changed(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    char *sfilter = (char *) gtk_entry_get_text(GTK_ENTRY(ctx->txt_filter));
    str_assign(ctx->view_filter, sfilter);

    cancel_wait_id(&ctx->view_wait_id);
    ctx->view_wait_id = g_timeout_add(200, tv_apply_filter, data);
}

static gboolean tv_apply_filter(gpointer data) {
    uictx_t *ctx = data;

    tv_refresh(GTK_TREE_VIEW(ctx->expenses_view), ctx, FALSE);
    ctx->view_wait_id = 0;
    return G_SOURCE_REMOVE;
}

void tv_add_expense_row(uictx_t *ctx) {
    exp_t *xp;
    ExpenseEditDialog *d;
    int z;
    GtkTreeIter it;
    GtkListStore *ls = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(ctx->expenses_view)));
    db_t *db = ctx->db;

    xp = exp_new();
    d = expeditdlg_new(ctx->db, xp);
    z = gtk_dialog_run(d->dlg);
    if (z == GTK_RESPONSE_OK) {
        expeditdlg_get_expense(d, ctx->db, xp);
        db_add_expense(ctx->db, xp);

        ctx->view_year = date_year(xp->dt);
        ctx->view_month = date_month(xp->dt);

        tv_refresh(GTK_TREE_VIEW(ctx->expenses_view), ctx, FALSE);
        tv_set_cursor_to_rowid(GTK_TREE_VIEW(ctx->expenses_view), xp->rowid);
        sidebar_refresh(ctx);
    }
    expeditdlg_free(d);
    exp_free(xp);
}

static void tv_edit_expense_row(GtkTreeView *tv, GtkTreeIter *it, uictx_t *ctx) {
    exp_t *xp;
    ExpenseEditDialog *d;
    int z;
    uint rowid;
    db_t *db = ctx->db;

    xp = exp_new();
    tv_get_expense(gtk_tree_view_get_model(tv), it, xp);
    rowid = xp->rowid;

    d = expeditdlg_new(db, xp);
    z = gtk_dialog_run(d->dlg);
    if (z == GTK_RESPONSE_OK) {
        expeditdlg_get_expense(d, db, xp);
        assert(xp->rowid == rowid);
        db_update_expense(db, xp);

        tv_set_expense(GTK_LIST_STORE(gtk_tree_view_get_model(tv)), it, db, xp);
        //tv_refresh(GTK_TREE_VIEW(ctx->expenses_view), ctx, TRUE);
        sidebar_refresh(ctx);
    }
    expeditdlg_free(d);
    exp_free(xp);
}
static void tv_del_expense_row(GtkTreeView *tv, GtkTreeIter *it, uictx_t *ctx) {
    exp_t *xp;
    GtkWidget *dlg;
    int z;
    db_t *db = ctx->db;

    xp = exp_new();
    tv_get_expense(gtk_tree_view_get_model(tv), it, xp);
    dlg = expdeldlg_new(db, xp);
    z = gtk_dialog_run(GTK_DIALOG(dlg));
    if (z == GTK_RESPONSE_OK) {
        db_del_expense(db, xp);
        gtk_list_store_remove(GTK_LIST_STORE(gtk_tree_view_get_model(tv)), it);
        sidebar_refresh(ctx);
    }

    exp_free(xp);
    gtk_widget_destroy(dlg);
}

static int cat_row_from_id(db_t *db, uint catid) {
    cat_t *cat;
    for (int i=0; i < db->cats->len; i++) {
        cat = db->cats->items[i];
        if (catid == cat->id)
            return i;
    }
    return 0;
}
static uint cat_id_from_row(db_t *db, int row) {
    if (row < 0 || row >= db->cats->len)
        return 0;

    cat_t *cat = db->cats->items[row];
    return cat->id;
}

static ExpenseEditDialog *expeditdlg_new(db_t *db, exp_t *xp) {
    ExpenseEditDialog *d;
    GtkWidget *dlg;
    GtkWidget *dlgbox;
    GtkWidget *lbl_date, *lbl_desc, *lbl_amt, *lbl_cat;
    GtkWidget *txt_date, *txt_desc, *txt_amt;
    GtkWidget *cbcat;
    GtkWidget *cal;
    GtkWidget *vboxdate;
    GtkWidget *tbl;
    GtkWidget *btnok, *btncancel;
    char samt[12];
    char isodate[ISO_DATE_LEN+1];
    cat_t *cat;

    dlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dlg), "Edit Expense");
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);

    lbl_date = gtk_label_new("Date");
    lbl_desc = gtk_label_new("Description");
    lbl_amt = gtk_label_new("Amount");
    lbl_cat = gtk_label_new("Category");
    g_object_set(lbl_date, "xalign", 0.0, NULL);
    //$$ align to top doesn't work when cal shown!
    g_object_set(lbl_date, "yalign", 0.0, NULL);
    g_object_set(lbl_desc, "xalign", 0.0, NULL);
    g_object_set(lbl_amt, "xalign", 0.0, NULL);
    g_object_set(lbl_cat, "xalign", 0.0, NULL);

    txt_date = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(txt_date), "YYYY-MM-DD");
    gtk_entry_set_icon_from_icon_name(GTK_ENTRY(txt_date), GTK_ENTRY_ICON_SECONDARY, "x-office-calendar-symbolic");
    cal = gtk_calendar_new();
    vboxdate = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vboxdate), txt_date, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vboxdate), cal, FALSE, FALSE, 0);

    txt_desc = gtk_entry_new();
    txt_amt = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(txt_desc), 25);

    date_to_iso(xp->dt, isodate, sizeof(isodate));
    gtk_entry_set_text(GTK_ENTRY(txt_date), isodate); 
    gtk_entry_set_text(GTK_ENTRY(txt_desc), xp->desc->s); 
    snprintf(samt, sizeof(samt), "%.2f", xp->amt);
    gtk_entry_set_text(GTK_ENTRY(txt_amt), samt); 

    cbcat = gtk_combo_box_text_new();
    for (int i=0; i < db->cats->len; i++) {
        cat = db->cats->items[i];
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cbcat), cat->name->s);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(cbcat), cat_row_from_id(db, xp->catid));

    gtk_calendar_select_month(GTK_CALENDAR(cal), date_month(xp->dt)-1, date_year(xp->dt));
    gtk_calendar_select_day(GTK_CALENDAR(cal), date_day(xp->dt));

    tbl = gtk_table_new(4, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(tbl), 5);
    gtk_table_set_col_spacings(GTK_TABLE(tbl), 10);
    gtk_container_set_border_width(GTK_CONTAINER(tbl), 5);
    gtk_table_attach(GTK_TABLE(tbl), lbl_date, 0,1, 0,1, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), lbl_desc, 0,1, 1,2, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), lbl_amt,  0,1, 2,3, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), lbl_cat,  0,1, 3,4, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), vboxdate, 1,2, 0,1, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), txt_desc, 1,2, 1,2, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), txt_amt,  1,2, 2,3, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), cbcat,  1,2, 3,4, GTK_FILL, GTK_SHRINK, 0,0);

    dlgbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_box_pack_start(GTK_BOX(dlgbox), tbl, TRUE, TRUE, 0);

    btnok = gtk_button_new_with_label("OK");
    btncancel = gtk_button_new_with_label("Cancel");
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), btnok, GTK_RESPONSE_OK);
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), btncancel, GTK_RESPONSE_CANCEL);

    gtk_widget_show_all(dlg);
    gtk_widget_hide(cal);

    d = malloc(sizeof(ExpenseEditDialog));
    d->dlg = GTK_DIALOG(dlg);
    d->txt_date = GTK_ENTRY(txt_date);
    d->cal = GTK_CALENDAR(cal);
    d->txt_desc = GTK_ENTRY(txt_desc);
    d->txt_amt = GTK_ENTRY(txt_amt);
    d->cbcat = GTK_COMBO_BOX(cbcat);
    d->showcal = 0;

    g_signal_connect(txt_amt, "insert-text", G_CALLBACK(expeditdlg_amt_insert_text_event), NULL);
    g_signal_connect(txt_date, "icon-press", G_CALLBACK(expeditdlg_date_icon_press_event), d);
//    g_signal_connect(txt_date, "insert-text", G_CALLBACK(expeditdlg_date_insert_text_event), NULL);
//    g_signal_connect(txt_date, "delete-text", G_CALLBACK(expeditdlg_date_delete_text_event), NULL);
//    g_signal_connect(txt_date, "key-press-event", G_CALLBACK(expeditdlg_date_key_press_event), NULL);
    g_signal_connect(cal, "day-selected", G_CALLBACK(expeditdlg_cal_day_selected_event), d);
    g_signal_connect(cal, "day-selected-double-click", G_CALLBACK(expeditdlg_cal_day_selected_dblclick_event), d);

    return d;
}
static void expeditdlg_free(ExpenseEditDialog *d) {
    gtk_widget_destroy(GTK_WIDGET(d->dlg));
    free(d);
}

static void expeditdlg_get_expense(ExpenseEditDialog *d, db_t *db, exp_t *xp) {
    const gchar *sdate = gtk_entry_get_text(d->txt_date);
    const gchar *sdesc = gtk_entry_get_text(d->txt_desc);
    const gchar *samt = gtk_entry_get_text(d->txt_amt);
    int cbrow;

    if (date_assign_iso(xp->dt, (char*) sdate) != 0)
        date_assign_time(xp->dt, time(NULL));
    if (strlen(sdesc) == 0)
        sdesc = "New Expense";
    str_assign(xp->desc, (char*)sdesc);
    xp->amt = atof(samt);
    xp->catid = cat_id_from_row(db, gtk_combo_box_get_active(d->cbcat));
}

static void expeditdlg_amt_insert_text_event(GtkEntry* ed, gchar *new_txt, gint len, gint *pos, gpointer data) {
    gchar new_ch;

    if (strlen(new_txt) > 1)
        return;
    new_ch = new_txt[0];

    // Only allow 0-9 or '.'
    if (!isdigit(new_ch) && new_ch != '.') {
        g_signal_stop_emission_by_name(G_OBJECT(ed), "insert-text");
        return;
    }

    // Only allow one '.' in entry
    if (new_ch == '.') {
        const gchar *cur_txt = gtk_entry_get_text(ed);
        if (strchr(cur_txt, '.') != NULL) {
            g_signal_stop_emission_by_name(G_OBJECT(ed), "insert-text");
            return;
        }
    }
}
static void expeditdlg_date_icon_press_event(GtkEntry *ed, GtkEntryIconPosition pos, GdkEvent *e, gpointer data) {
    ExpenseEditDialog *d = data;

    // Toggle visibility of calendar control.
    d->showcal = !d->showcal;
    if (d->showcal)
        gtk_widget_show(GTK_WIDGET(d->cal));
    else
        gtk_widget_hide(GTK_WIDGET(d->cal));
}
static void expeditdlg_date_insert_text_event(GtkEntry *ed, gchar *newtxt, gint newtxt_len, gint *pos, gpointer data) {
    int icur = *pos;
    gchar newch;
    const gchar *curtxt;
    size_t curtxt_len;
    gchar buf[ISO_DATE_LEN+1];

    if (newtxt_len > 1)
        return;
    newch = newtxt[0];

    if (!isdigit(newch)) {
        g_signal_stop_emission_by_name(G_OBJECT(ed), "insert-text");
        return;
    }

    curtxt = gtk_entry_get_text(ed);
    curtxt_len = strlen(curtxt);

    if (icur >= ISO_DATE_LEN) {
        g_signal_stop_emission_by_name(G_OBJECT(ed), "insert-text");
        return;
    }

    strncpy(buf, curtxt, curtxt_len);
    buf[curtxt_len] = 0;

    // 2023-12-25
    // 0123456789
    // 1234567890

    if (icur == 3 || icur == 6) {
        buf[icur] = newch;
        icur++;
        buf[icur] = '-';
    } else if (icur == 4 || icur == 7) {
        buf[icur] = '-';
        icur++;
        buf[icur] = newch;
    } else {
        buf[icur] = newch;
    }
    icur++;
    *pos = icur;

    if (icur > curtxt_len-1)
        buf[icur] = 0;

    g_signal_handlers_block_by_func(G_OBJECT(ed), G_CALLBACK(expeditdlg_date_insert_text_event), data);
    gtk_entry_set_text(ed, buf);
    g_signal_handlers_unblock_by_func(G_OBJECT(ed), G_CALLBACK(expeditdlg_date_insert_text_event), data);
    g_signal_stop_emission_by_name(G_OBJECT(ed), "insert-text");
}
static void expeditdlg_date_delete_text_event(GtkEntry* ed, gint startpos, gint endpos, gpointer data) {
    const gchar *curtxt;
    size_t curtxt_len;
    gint del_len;

    curtxt = gtk_entry_get_text(ed);
    curtxt_len = strlen(curtxt);

    if (endpos == -1)
        endpos = curtxt_len;
    del_len = endpos - startpos;

    // Can't delete more than one char unless entire text is to be deleted.
    if (del_len > 1 && del_len != curtxt_len) {
        g_signal_stop_emission_by_name(G_OBJECT(ed), "delete-text");
        return;
    }
}
static gboolean expeditdlg_date_key_press_event(GtkEntry *ed, GdkEventKey *e, gpointer data) {
    if (e->keyval == GDK_KEY_Delete || e->keyval == GDK_KEY_KP_Delete)
        return TRUE;

    return FALSE;
}
static void expeditdlg_cal_day_selected_event(GtkCalendar *cal, gpointer data) {
    ExpenseEditDialog *d = data;
    date_t *dt;
    uint year, month, day;
    char isodate[ISO_DATE_LEN+1];

    gtk_calendar_get_date(d->cal, &year, &month, &day);
    month += 1;
    dt = date_new_cal(year, month, day);

    date_to_iso(dt, isodate, sizeof(isodate));
    gtk_entry_set_text(d->txt_date, isodate); 
    date_free(dt);
}
static void expeditdlg_cal_day_selected_dblclick_event(GtkCalendar *cal, gpointer data) {
    ExpenseEditDialog *d = data;

    expeditdlg_cal_day_selected_event(cal, data);
    d->showcal = 0;
    gtk_widget_hide(GTK_WIDGET(d->cal));
}

static GtkWidget *expdeldlg_new(db_t *db, exp_t *xp) {
    GtkWidget *dlg;
    GtkWidget *dlgbox;
    GtkWidget *lbl_date, *lbl_desc, *lbl_amt, *lbl_cat;
    GtkWidget *txt_date, *txt_desc, *txt_amt, *txt_cat;
    GtkWidget *tbl;
    GtkWidget *btnok, *btncancel;
    char samt[12];
    char isodate[ISO_DATE_LEN+1];
    cat_t *cat;

    dlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dlg), "Delete Expense");
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);

    date_to_iso(xp->dt, isodate, sizeof(isodate));
    snprintf(samt, sizeof(samt), "%.2f", xp->amt);

    lbl_date = gtk_label_new("Date");
    lbl_desc = gtk_label_new("Description");
    lbl_amt = gtk_label_new("Amount");
    lbl_cat = gtk_label_new("Category");
    txt_date = gtk_entry_new();
    txt_desc = gtk_entry_new();
    txt_amt = gtk_entry_new();
    txt_cat = gtk_entry_new();
    g_object_set(lbl_date, "xalign", 0.0, NULL);
    g_object_set(lbl_desc, "xalign", 0.0, NULL);
    g_object_set(lbl_amt, "xalign", 0.0, NULL);
    g_object_set(lbl_cat, "xalign", 0.0, NULL);

    gtk_entry_set_text(GTK_ENTRY(txt_date), isodate); 
    gtk_entry_set_text(GTK_ENTRY(txt_desc), xp->desc->s); 
    gtk_entry_set_text(GTK_ENTRY(txt_amt), samt); 
    gtk_entry_set_text(GTK_ENTRY(txt_cat), db_find_cat_name(db, xp->catid)); 

    gtk_widget_set_sensitive(txt_date, FALSE);
    gtk_widget_set_sensitive(txt_desc, FALSE);
    gtk_widget_set_sensitive(txt_amt, FALSE);
    gtk_widget_set_sensitive(txt_cat, FALSE);

    tbl = gtk_table_new(4, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(tbl), 5);
    gtk_table_set_col_spacings(GTK_TABLE(tbl), 10);
    gtk_container_set_border_width(GTK_CONTAINER(tbl), 5);
    gtk_table_attach(GTK_TABLE(tbl), lbl_date, 0,1, 0,1, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), lbl_desc, 0,1, 1,2, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), lbl_amt,  0,1, 2,3, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), lbl_cat,  0,1, 3,4, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), txt_date, 1,2, 0,1, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), txt_desc, 1,2, 1,2, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), txt_amt,  1,2, 2,3, GTK_FILL, GTK_SHRINK, 0,0);
    gtk_table_attach(GTK_TABLE(tbl), txt_cat,  1,2, 3,4, GTK_FILL, GTK_SHRINK, 0,0);

    dlgbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_box_pack_start(GTK_BOX(dlgbox), tbl, TRUE, TRUE, 0);

    btnok = gtk_button_new_with_label("Delete");
    btncancel = gtk_button_new_with_label("Cancel");
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), btnok, GTK_RESPONSE_OK);
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), btncancel, GTK_RESPONSE_CANCEL);

    gtk_widget_show_all(dlg);
    return dlg;
}

static GtkWidget *cateditdlg_new(db_t *db) {
    GtkWidget *dlg;
    GtkWidget *dlgbox;
    GtkWidget *tv;
    GtkWidget *sw;
    GtkTreeViewColumn *col;
    GtkCellRenderer *r;
    GtkListStore *ls;
    GtkTreeIter it;
    GtkWidget *btnadd, *btnclose;
    int xpadding = 10;
    int ypadding = 2;
    cat_t *cat;

    dlg = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dlg), "Categories");
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);

    tv = gtk_tree_view_new();
    sw = create_scroll_window(tv);
    g_object_set(tv, "enable-search", FALSE, NULL);
    gtk_widget_add_events(tv, GDK_KEY_PRESS_MASK);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("ID", r, "text", CAT_FIELD_ID, NULL);
    gtk_tree_view_column_set_sort_column_id(col, CAT_FIELD_ID);
    gtk_tree_view_column_set_resizable(col, FALSE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    g_object_set(r, "editable", TRUE, "editable-set", TRUE, NULL);
    col = gtk_tree_view_column_new_with_attributes("Name", r, "text", CAT_FIELD_NAME, NULL);
    gtk_tree_view_column_set_sort_column_id(col, CAT_FIELD_NAME);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width(col, 200);
    gtk_tree_view_column_set_expand(col, FALSE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    ls = gtk_list_store_new(CAT_FIELD_NCOLS,
                            G_TYPE_UINT,   // CAT_FIELD_ID
                            G_TYPE_STRING  // CAT_FIELD_NAME
                            );
    gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ls));
    g_object_unref(ls);

    for (int i=0; i < db->cats->len; i++) {
        cat = db->cats->items[i];
        gtk_list_store_append(ls, &it);
        gtk_list_store_set(ls, &it,
                           CAT_FIELD_ID, cat->id,
                           CAT_FIELD_NAME, cat->name->s,
                           -1);
    }

    dlgbox = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_box_pack_start(GTK_BOX(dlgbox), sw, TRUE, TRUE, 0);
    gtk_widget_set_size_request(dlgbox, 250, 250);

    btnadd = gtk_button_new_with_label("Add");
    btnclose = gtk_button_new_with_label("Close");
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), btnadd, 1);
    gtk_dialog_add_action_widget(GTK_DIALOG(dlg), btnclose, GTK_RESPONSE_OK);

    g_signal_connect(G_OBJECT(r), "edited", G_CALLBACK(cateditdlg_name_edited), tv);
    g_signal_connect(tv, "row-activated", G_CALLBACK(cateditdlg_row_activated), NULL);
    g_signal_connect(btnadd, "clicked", G_CALLBACK(cateditdlg_add_clicked), tv);

    gtk_widget_show_all(dlg);
    return dlg;
}
static void cateditdlg_name_edited(GtkCellRendererText *r, gchar *path, gchar *newname, GtkTreeView *tv) {
    GtkTreeIter it;
    GtkListStore *ls;

    if (strlen(newname) == 0)
        return;

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(tv));
    if (!gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ls), &it, path))
        return;
    gtk_list_store_set(ls, &it, CAT_FIELD_NAME, newname, -1);
}
static void cateditdlg_row_activated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer data) {
    GtkTreeViewColumn *col1 = gtk_tree_view_get_column(tv, CAT_FIELD_NAME);
    gtk_tree_view_set_cursor(tv, tp, col1, TRUE);
}
static void cateditdlg_add_clicked(GtkWidget *w, GtkTreeView *tv) {
    printf("add clicked\n");
    g_signal_stop_emission_by_name(G_OBJECT(w), "clicked");
}

static void copy_year_str(int year, char *syear, size_t syear_len) {
    if (year == 0)
        strncpy(syear, "All", syear_len);
    else
        snprintf(syear, syear_len, "%d", year);
}
static char *get_month_name(uint month) {
    assert(month < countof(_month_names));
    return _month_names[month];
}

static GtkWidget *create_year_menuitem(int year) {
    char syear[5];
    copy_year_str(year, syear, sizeof(syear));
    return gtk_menu_item_new_with_label(syear);
}
static GtkWidget *create_month_menuitem(int month) {
    return gtk_menu_item_new_with_label(get_month_name(month));
}

GtkWidget *sidebar_new(uictx_t *ctx) {
    GtkWidget *frame;
    GtkWidget *hbox;
    GtkWidget *yearbtnlabel;
    GtkWidget *monthbtnlabel;
    GtkWidget *yearbtn;
    GtkWidget *monthbtn;
    GtkWidget *menubtn_hbox;
    GtkWidget *icon;
    GtkWidget *yearmenu;
    GtkWidget *monthmenu;
    GtkWidget *mi;
    GtkWidget *addbtn, *editbtn, *delbtn;
    char syear[5];

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    g_object_set(hbox, "margin-top", 2, "margin-bottom", 2, NULL);

    // Year selector menu
    yearmenu = gtk_menu_new();
    sidebar_populate_year_menu(yearmenu, ctx);

    copy_year_str(ctx->view_year, syear, sizeof(syear));
    yearbtnlabel = gtk_label_new(syear);
    icon = gtk_image_new_from_icon_name("pan-up-symbolic", GTK_ICON_SIZE_BUTTON);
    menubtn_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(menubtn_hbox), yearbtnlabel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(menubtn_hbox), icon, FALSE, FALSE, 0);

    yearbtn = gtk_menu_button_new();
    gtk_container_add(GTK_CONTAINER(yearbtn), menubtn_hbox);
    gtk_menu_button_set_direction(GTK_MENU_BUTTON(yearbtn), GTK_ARROW_UP);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(yearbtn), yearmenu);
    g_object_set(yearmenu, "halign", GTK_ALIGN_END, NULL);

    // Month selector menu
    monthmenu = gtk_menu_new();
    sidebar_populate_month_menu(monthmenu, ctx);

    monthbtnlabel = gtk_label_new(get_month_name(ctx->view_month));
    icon = gtk_image_new_from_icon_name("pan-up-symbolic", GTK_ICON_SIZE_BUTTON);
    menubtn_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(menubtn_hbox), monthbtnlabel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(menubtn_hbox), icon, FALSE, FALSE, 0);

    monthbtn = gtk_menu_button_new();
    gtk_container_add(GTK_CONTAINER(monthbtn), menubtn_hbox);
    gtk_menu_button_set_direction(GTK_MENU_BUTTON(monthbtn), GTK_ARROW_UP);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(monthbtn), monthmenu);
    g_object_set(monthmenu, "halign", GTK_ALIGN_END, NULL);

    gtk_box_pack_start(GTK_BOX(hbox), yearbtn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), monthbtn, FALSE, FALSE, 0);

    addbtn = gtk_button_new_from_icon_name("list-add-symbolic", GTK_ICON_SIZE_BUTTON);
    editbtn = gtk_button_new_from_icon_name("document-edit-symbolic", GTK_ICON_SIZE_BUTTON);
    delbtn = gtk_button_new_from_icon_name("edit-delete-symbolic", GTK_ICON_SIZE_BUTTON);
    //addbtn = gtk_button_new_with_mnemonic("_Add");
    //editbtn = gtk_button_new_with_mnemonic("_Edit");
    //delbtn = gtk_button_new_with_mnemonic("_Delete");
    gtk_box_pack_end(GTK_BOX(hbox), delbtn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), editbtn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), addbtn, FALSE, FALSE, 0);

    ctx->yearmenu = yearmenu;
    ctx->yearbtn = yearbtn;
    ctx->monthbtn = monthbtn;
    ctx->yearbtnlabel = yearbtnlabel;
    ctx->monthbtnlabel = monthbtnlabel;

    //return create_frame("", hbox, 4, 0);
    return hbox;
}

static void sidebar_refresh(uictx_t *ctx) {
    char syear[5];

    sidebar_populate_year_menu(ctx->yearmenu, ctx);

    copy_year_str(ctx->view_year, syear, sizeof(syear));
    gtk_label_set_label(GTK_LABEL(ctx->yearbtnlabel), syear);
    gtk_label_set_label(GTK_LABEL(ctx->monthbtnlabel), get_month_name(ctx->view_month));
    gtk_entry_set_text(GTK_ENTRY(ctx->txt_filter), ctx->view_filter->s);
}

static void sidebar_populate_year_menu(GtkWidget *menu, uictx_t *ctx) {
    GtkWidget *mi;
    ulong year;
    intarray_t *years = ctx->db->years;

    remove_children(menu);
    for (int i=0; i < years->len; i++) {
        year = years->items[i];
        mi = create_year_menuitem(year);
        g_object_set_data(G_OBJECT(mi), "tag", (gpointer) year);
        g_signal_connect(mi, "activate", G_CALLBACK(yearmenu_select), ctx);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }
    gtk_widget_show_all(menu);
}
static void sidebar_populate_month_menu(GtkWidget *menu, uictx_t *ctx) {
    GtkWidget *mi;

    for (ulong i=0; i <= 12; i++) {
        mi = create_month_menuitem(i);
        g_object_set_data(G_OBJECT(mi), "tag", (gpointer) i);
        g_signal_connect(mi, "activate", G_CALLBACK(monthmenu_select), ctx);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }
    gtk_widget_show_all(menu);
}
static void yearmenu_select(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;

    const gchar *mitext = gtk_menu_item_get_label(GTK_MENU_ITEM(w));
    gtk_label_set_label(GTK_LABEL(ctx->yearbtnlabel), mitext);

    ulong year = (ulong) g_object_get_data(G_OBJECT(w), "tag");
    ctx->view_year = (uint)year;
    ctx->view_month = 0;
    gtk_label_set_label(GTK_LABEL(ctx->monthbtnlabel), get_month_name(ctx->view_month));

    tv_refresh(GTK_TREE_VIEW(ctx->expenses_view), ctx, FALSE);
}
static void monthmenu_select(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    ulong month = (ulong) g_object_get_data(G_OBJECT(w), "tag");

    const gchar *mitext = gtk_menu_item_get_label(GTK_MENU_ITEM(w));
    gtk_label_set_label(GTK_LABEL(ctx->monthbtnlabel), mitext);

    ctx->view_month = (uint)month;
    tv_refresh(GTK_TREE_VIEW(ctx->expenses_view), ctx, FALSE);
}

