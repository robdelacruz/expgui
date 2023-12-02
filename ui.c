#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>

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

// Main menu bar
static GtkWidget *mainmenu_new(uictx_t *ctx, GtkWidget *mainwin);
static void mainmenu_file_open(GtkWidget *w, gpointer data);
static void mainmenu_file_new(GtkWidget *w, gpointer data);
static void mainmenu_file_save(GtkWidget *w, gpointer data);
static void mainmenu_file_saveas(GtkWidget *w, gpointer data);
static void mainmenu_expense_add(GtkWidget *w, gpointer data);
static void mainmenu_expense_edit(GtkWidget *w, gpointer data);
static void mainmenu_expense_delete(GtkWidget *w, gpointer data);

// Expenses Treeview
GtkWidget *expensestv_new(uictx_t *ctx);
static void expensestv_amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data);
static void expensestv_row_activated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer data);
static void expensestv_row_changed(GtkTreeSelection *ts, gpointer data);
static gboolean expensestv_keypress(GtkTreeView *tv, GdkEventKey *e, gpointer data);

static void expensestv_get_expense(GtkTreeView *tv, GtkTreeIter *it, exp_t *xp);
static void expensestv_refresh(GtkTreeView *tv, db_t *db, gboolean reset_cursor);
static void expensestv_filter_changed(GtkWidget *w, gpointer data);
static gboolean expensestv_apply_filter(gpointer data);

static void expensestv_add_expense_row(uictx_t *ctx);
static void expensestv_edit_expense_row(GtkTreeView *tv, GtkTreeIter *it, uictx_t *ctx);

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

uictx_t *uictx_new() {
    uictx_t *ctx = malloc(sizeof(uictx_t));

    ctx->xpfile = str_new(0);
    ctx->db = db_new();

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
    db_free(ctx->db);
    free(ctx);
}
void uictx_reset(uictx_t *ctx) {
    str_assign(ctx->xpfile, "");
    db_reset(ctx->db);
}

void uictx_setup_ui(uictx_t *ctx) {
    GtkWidget *mainwin;
    GtkWidget *menubar;

    GtkWidget *expenses_sw;
    GtkWidget *expenses_view;
    GtkWidget *expenses_frame;
    GtkWidget *sidebar;
    GtkWidget *main_vbox;

    // mainwin
    mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainwin), "Expense Buddy");
    //gtk_widget_set_size_request(mainwin, 800, 600);
    gtk_window_set_default_size(GTK_WINDOW(mainwin), 480, 480);

    menubar = mainmenu_new(ctx, mainwin);
    expenses_frame = expensestv_new(ctx);
    sidebar = sidebar_new(ctx);

    // Main window
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), expenses_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), sidebar, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(mainwin), main_vbox);
    gtk_widget_show_all(mainwin);

    g_signal_connect(mainwin, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_grab_focus(ctx->expenses_view);

    ctx->mainwin = mainwin;
}

int uictx_open_expense_file(uictx_t *ctx, char *xpfile) {
    FILE *f;
    char *filter;

    f = fopen(xpfile, "r");
    if (f == NULL)
        return 1;

    uictx_reset(ctx);
    db_load_expense_file(ctx->db, f);
    str_assign(ctx->xpfile, xpfile);
    fclose(f);

    expensestv_refresh(GTK_TREE_VIEW(ctx->expenses_view), ctx->db, TRUE);
    sidebar_refresh(ctx);

    return 0;
}

static GtkWidget *mainmenu_new(uictx_t *ctx, GtkWidget *mainwin) {
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

    g_signal_connect(mi_file_new, "activate", G_CALLBACK(mainmenu_file_new), ctx);
    g_signal_connect(mi_file_open, "activate", G_CALLBACK(mainmenu_file_open), ctx);
    g_signal_connect(mi_file_save, "activate", G_CALLBACK(mainmenu_file_save), ctx);
    g_signal_connect(mi_file_saveas, "activate", G_CALLBACK(mainmenu_file_saveas), ctx);

    g_signal_connect(mi_expense_add, "activate", G_CALLBACK(mainmenu_expense_add), ctx);
    g_signal_connect(mi_expense_edit, "activate", G_CALLBACK(mainmenu_expense_edit), ctx);
    g_signal_connect(mi_expense_delete, "activate", G_CALLBACK(mainmenu_expense_delete), ctx);

    g_signal_connect(mi_file_quit, "activate", G_CALLBACK(gtk_main_quit), NULL);

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
    z = uictx_open_expense_file(ctx, xpfile);
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
}
static void mainmenu_file_saveas(GtkWidget *w, gpointer data) {
}
static void mainmenu_expense_add(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    expensestv_add_expense_row(ctx);
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
    expensestv_edit_expense_row(tv, &it, ctx);
}
static void mainmenu_expense_delete(GtkWidget *w, gpointer data) {
}

enum exp_field_t {
    EXP_FIELD_ROWID = 0,
    EXP_FIELD_DATE,
    EXP_FIELD_TIME,
    EXP_FIELD_DESC,
    EXP_FIELD_AMT,
    EXP_FIELD_CATID,
    EXP_FIELD_CATNAME
};


GtkWidget *expensestv_new(uictx_t *ctx) {
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
    gtk_tree_view_column_set_cell_data_func(col, r, expensestv_amt_datafunc, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Category", r, "text", EXP_FIELD_CATNAME, NULL);
    gtk_tree_view_column_set_sort_column_id(col, EXP_FIELD_CATNAME);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    ls = gtk_list_store_new(7,
                            G_TYPE_UINT,   // EXP_FIELD_ROWID
                            G_TYPE_STRING, // EXP_FIELD_DATE
                            G_TYPE_STRING, // EXP_FIELD_TIME
                            G_TYPE_STRING, // EXP_FIELD_DESC
                            G_TYPE_DOUBLE, // EXP_FIELD_AMT
                            G_TYPE_UINT,   // EXP_FIELD_CATID
                            G_TYPE_STRING  // EXP_FIELD_CATNAME
                            );
    gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ls));
    g_object_unref(ls);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(ls), 0, GTK_SORT_DESCENDING);

    txt_filter = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(txt_filter), "Filter Expenses");

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), txt_filter, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    frame = create_frame("", vbox, 4, 0);

    g_signal_connect(tv, "row-activated", G_CALLBACK(expensestv_row_activated), ctx);
//    g_signal_connect(ts, "changed", G_CALLBACK(expensestv_row_changed), ctx);
    g_signal_connect(tv, "key-press-event", G_CALLBACK(expensestv_keypress), ctx);
    g_signal_connect(txt_filter, "changed", G_CALLBACK(expensestv_filter_changed), ctx);

    ctx->expenses_view = tv;
    ctx->txt_filter = txt_filter;
    return frame;
}

static void expensestv_amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data) {
    gdouble amt;
    char buf[15];

    gtk_tree_model_get(m, it, EXP_FIELD_AMT, &amt, -1);
    snprintf(buf, sizeof(buf), "%9.2f", amt);
    g_object_set(r, "text", buf, NULL);
}

static void expensestv_row_activated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer data) {
    uictx_t *ctx = data;
    GtkTreeSelection *ts;
    GtkTreeModel *m;
    GtkTreeIter it;

    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    if (!gtk_tree_selection_get_selected(ts, &m, &it))
        return;
    expensestv_edit_expense_row(tv, &it, ctx);
}

static void expensestv_row_changed(GtkTreeSelection *ts, gpointer data) {
    uictx_t *ctx = data;
    GtkTreeView *tv;
    GtkListStore *ls;
    GtkTreeIter it;

    if (!gtk_tree_selection_get_selected(ts, (GtkTreeModel **)&ls, &it))
        return;
    tv = gtk_tree_selection_get_tree_view(ts);
}

static gboolean expensestv_keypress(GtkTreeView *tv, GdkEventKey *e, gpointer data) {
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
    } else if (kv == GDK_KEY_e) {
        expensestv_edit_expense_row(tv, &it, ctx);
        return TRUE;
    } else if (kv == GDK_KEY_Delete || (e->state == GDK_CONTROL_MASK && kv == GDK_KEY_x)) {
    }

    return FALSE;
}

static void expensestv_get_expense(GtkTreeView *tv, GtkTreeIter *it, exp_t *xp) {
    GtkListStore *ls;
    gchar *sdate;
    gchar *desc;
    gdouble amt;
    uint catid;
    uint rowid;

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(tv));
    gtk_tree_model_get(GTK_TREE_MODEL(ls), it,
                       EXP_FIELD_ROWID, &rowid,
                       EXP_FIELD_DATE, &sdate,
                       EXP_FIELD_DESC, &desc,
                       EXP_FIELD_AMT, &amt,
                       EXP_FIELD_CATID, &catid,
                       -1);
    xp->rowid = rowid;
    xp->dt = date_from_iso(sdate);
    str_assign(xp->time, "");
    str_assign(xp->desc, desc);
    xp->amt = amt;
    xp->catid = catid;

    g_free(sdate);
    g_free(desc);
}

static void expensestv_refresh(GtkTreeView *tv, db_t *db, gboolean reset_cursor) {
    GtkListStore *ls;
    GtkTreeIter it;
    GtkTreePath *tp;
    GtkTreeSelection *ts;
    char isodate[ISO_DATE_LEN+1];
    exp_t *xp;
    uint cur_rowid = 0;

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
    assert(ls != NULL);

    // (a) Remember active row to be restored later.
    gtk_tree_view_get_cursor(tv, &tp, NULL);
    if (tp) {
        gtk_tree_model_get_iter(GTK_TREE_MODEL(ls), &it, tp);
        gtk_tree_model_get(GTK_TREE_MODEL(ls), &it, 4, &cur_rowid, -1);
    }
    gtk_tree_path_free(tp);

    // Turn off selection while refreshing treeview so we don't get
    // bombarded by 'change' events.
    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    gtk_tree_selection_set_mode(ts, GTK_SELECTION_NONE);

    gtk_list_store_clear(ls);

    for (int i=0; i < db->view_xps->len; i++) {
        xp = db->view_xps->items[i];
        date_to_iso(xp->dt, isodate, sizeof(isodate));

        gtk_list_store_append(ls, &it);
        gtk_list_store_set(ls, &it, 
                           EXP_FIELD_ROWID, xp->rowid,
                           EXP_FIELD_DATE, isodate,
                           EXP_FIELD_TIME, "",
                           EXP_FIELD_DESC, xp->desc->s,
                           EXP_FIELD_AMT, xp->amt,
                           EXP_FIELD_CATID, xp->catid,
                           EXP_FIELD_CATNAME, db_find_cat_name(db, xp->catid),
                           -1);

        // (a) Restore previously active row.
        if (xp->rowid == cur_rowid) {
            tp = gtk_tree_model_get_path(GTK_TREE_MODEL(ls), &it);
            gtk_tree_view_set_cursor(tv, tp, NULL, FALSE);
            gtk_tree_path_free(tp);
        }
    }

    gtk_tree_selection_set_mode(ts, GTK_SELECTION_BROWSE);

    if (reset_cursor) {
        tp = gtk_tree_path_new_from_string("0");
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(tv), tp, NULL, FALSE);
        gtk_tree_path_free(tp);
    }
}

static void expensestv_filter_changed(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    char *sfilter = (char *) gtk_entry_get_text(GTK_ENTRY(ctx->txt_filter));
    str_assign(ctx->db->view_filter, sfilter);

    cancel_wait_id(&ctx->view_wait_id);
    ctx->view_wait_id = g_timeout_add(200, expensestv_apply_filter, data);
}

static gboolean expensestv_apply_filter(gpointer data) {
    uictx_t *ctx = data;

    db_apply_filter(ctx->db);
    expensestv_refresh(GTK_TREE_VIEW(ctx->expenses_view), ctx->db, FALSE);

    ctx->view_wait_id = 0;
    return G_SOURCE_REMOVE;
}

void expensestv_add_expense_row(uictx_t *ctx) {
    exp_t *xp;
    ExpenseEditDialog *d;
    gint z;

    xp = exp_new();
    d = expeditdlg_new(ctx->db, xp);
    z = gtk_dialog_run(d->dlg);
    if (z == GTK_RESPONSE_OK) {
        expeditdlg_get_expense(d, ctx->db, xp);
        db_add_expense(ctx->db, xp);

        expensestv_apply_filter(ctx);
        expensestv_refresh(GTK_TREE_VIEW(ctx->expenses_view), ctx->db, FALSE);
        sidebar_populate_year_menu(ctx->yearmenu, ctx);
    }
    expeditdlg_free(d);
    exp_free(xp);
}

static void expensestv_edit_expense_row(GtkTreeView *tv, GtkTreeIter *it, uictx_t *ctx) {
    exp_t *xp;
    ExpenseEditDialog *d;
    gint z;
    uint rowid;

    xp = exp_new();
    expensestv_get_expense(tv, it, xp);
    rowid = xp->rowid;

    d = expeditdlg_new(ctx->db, xp);
    z = gtk_dialog_run(d->dlg);
    if (z == GTK_RESPONSE_OK) {
        expeditdlg_get_expense(d, ctx->db, xp);
        assert(xp->rowid == rowid);
        db_update_expense(ctx->db, xp);

        expensestv_apply_filter(ctx);
        expensestv_refresh(GTK_TREE_VIEW(ctx->expenses_view), ctx->db, FALSE);
        sidebar_populate_year_menu(ctx->yearmenu, ctx);
    }
    expeditdlg_free(d);
    exp_free(xp);
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
    char samt[12];
    char isodate[ISO_DATE_LEN+1];
    cat_t *cat;

    dlg = gtk_dialog_new_with_buttons("Edit Expense", NULL, GTK_DIALOG_MODAL,
                                      GTK_STOCK_OK, GTK_RESPONSE_OK,
                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                      NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_CANCEL);
    GtkWidget *btn_ok = gtk_dialog_get_widget_for_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK); 
    if (btn_ok)
        gtk_widget_grab_focus(btn_ok);

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

    gtk_calendar_select_month(GTK_CALENDAR(cal), xp->dt.month-1, xp->dt.year);
    gtk_calendar_select_day(GTK_CALENDAR(cal), xp->dt.day);

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
    g_signal_connect(txt_date, "insert-text", G_CALLBACK(expeditdlg_date_insert_text_event), NULL);
    g_signal_connect(txt_date, "delete-text", G_CALLBACK(expeditdlg_date_delete_text_event), NULL);
    g_signal_connect(txt_date, "key-press-event", G_CALLBACK(expeditdlg_date_key_press_event), NULL);
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

    xp->dt = date_from_iso((char*) sdate);
    str_assign(xp->time, "");
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
    date_t dt;
    char isodate[ISO_DATE_LEN+1];

    gtk_calendar_get_date(d->cal, &dt.year, &dt.month, &dt.day);
    dt.month += 1;

    date_to_iso(dt, isodate, sizeof(isodate));
    gtk_entry_set_text(d->txt_date, isodate); 
}
static void expeditdlg_cal_day_selected_dblclick_event(GtkCalendar *cal, gpointer data) {
    ExpenseEditDialog *d = data;

    expeditdlg_cal_day_selected_event(cal, data);
    d->showcal = 0;
    gtk_widget_hide(GTK_WIDGET(d->cal));
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

    copy_year_str(ctx->db->view_year, syear, sizeof(syear));
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

    monthbtnlabel = gtk_label_new(get_month_name(ctx->db->view_month));
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

    copy_year_str(ctx->db->view_year, syear, sizeof(syear));
    gtk_label_set_label(GTK_LABEL(ctx->yearbtnlabel), syear);
    gtk_label_set_label(GTK_LABEL(ctx->monthbtnlabel), get_month_name(ctx->db->view_month));
    gtk_entry_set_text(GTK_ENTRY(ctx->txt_filter), ctx->db->view_filter->s);
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
    ctx->db->view_year = (uint)year;
    ctx->db->view_month = 0;
    gtk_label_set_label(GTK_LABEL(ctx->monthbtnlabel), get_month_name(ctx->db->view_month));

    expensestv_apply_filter(ctx);
}
static void monthmenu_select(GtkWidget *w, gpointer data) {
    uictx_t *ctx = data;
    ulong month = (ulong) g_object_get_data(G_OBJECT(w), "tag");

    const gchar *mitext = gtk_menu_item_get_label(GTK_MENU_ITEM(w));
    gtk_label_set_label(GTK_LABEL(ctx->monthbtnlabel), mitext);

    ctx->db->view_month = (uint)month;
    expensestv_apply_filter(ctx);
}

