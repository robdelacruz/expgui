#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>

#include "ui.h"

static void set_screen_css(char *cssfile);
static GtkWidget *create_scroll_window(GtkWidget *child);
static GtkWidget *create_frame(gchar *label, GtkWidget *child, int xpadding, int ypadding);
static void add_accel(GtkWidget *w, GtkAccelGroup *a, guint key, GdkModifierType mods);
static void cancel_wait_id(guint *wait_id);

void setup_ui(uictx_t *ctx);

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
static void expensestv_refresh(GtkTreeView *tv, array_t *xps, gboolean reset_cursor);
static void expensestv_filter_changed(GtkWidget *w, gpointer data);
static gboolean expensestv_apply_filter(gpointer data);

static void expensestv_add_expense_row(uictx_t *ctx);
static void expensestv_edit_expense_row(GtkTreeView *tv, GtkTreeIter *it, uictx_t *ctx);

// Sidebar
static GtkWidget *create_sidebar_controls(uictx_t *ctx);


typedef struct {
    GtkDialog *dlg;
    GtkEntry *txt_date;
    GtkEntry *txt_desc;
    GtkEntry *txt_amt;
    GtkEntry *txt_cat;
} ExpenseEditDialog;

static ExpenseEditDialog *expeditdlg_new(exp_t *xp);
static void expeditdlg_free(ExpenseEditDialog *d);
static void expeditdlg_get_expense(ExpenseEditDialog *d, exp_t *xp);
static void expeditdlg_amt_insert_text_event(GtkEntry* ed, gchar *new_txt, gint len, gint *pos, gpointer data);
static void expeditdlg_date_insert_text_event(GtkEntry* ed, gchar *newtxt, gint newtxt_len, gint *pos, gpointer data);
static void expeditdlg_date_delete_text_event(GtkEntry* ed, gint startpos, gint endpos, gpointer data);
static gboolean expeditdlg_date_key_press_event(GtkEntry *ed, GdkEventKey *e, gpointer data);


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

uictx_t *uictx_new() {
    uictx_t *ctx = malloc(sizeof(uictx_t));

    ctx->xpfile = str_new(0);

    array_assign(&ctx->all_xps,
                 (void **)ctx->_XPS1, 0, countof(ctx->_XPS1));
    array_assign(&ctx->view_xps,
                 (void **)ctx->_XPS2, 0, countof(ctx->_XPS2));

    date_t today = current_date();
    ctx->view_year = today.year;
    ctx->view_month = today.month;
    ctx->view_month = 0;

    ctx->mainwin = NULL;
    ctx->expenses_view = NULL;
    ctx->txt_filter = NULL;

    return ctx;
}
void uictx_free(uictx_t *ctx) {
    str_free(ctx->xpfile);
    free(ctx);
}
void uictx_reset(uictx_t *ctx) {
    str_assign(ctx->xpfile, "");

    for (int i=0; i < ctx->all_xps.len; i++) {
        assert(ctx->all_xps.items[i] != NULL);
        exp_free(ctx->all_xps.items[i]);
    }

    array_clear(&ctx->all_xps);
    array_clear(&ctx->view_xps);

    date_t today = current_date();
    ctx->view_year = today.year;
    ctx->view_month = today.month;
    ctx->view_month = 0;
}

void setup_ui(uictx_t *ctx) {
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
    //gtk_widget_set_size_request(mainwin, 800, 600);
    gtk_window_set_default_size(GTK_WINDOW(mainwin), 480, 480);

    menubar = mainmenu_new(ctx, mainwin);
    statusbar = gtk_statusbar_new();
    gtk_widget_set_halign(statusbar, GTK_ALIGN_END);

    guint statusid =  gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "info");
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, "Expense Buddy GUI");

    expenses_frame = expensestv_new(ctx);
    sidebar = create_sidebar_controls(ctx);
    //g_object_set(sidebar, "margin-top", 5, NULL);
    //gtk_widget_set_size_request(sidebar, 100, -1);

    //hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    //gtk_box_pack_start(GTK_BOX(hbox1), sidebar, FALSE, FALSE, 0);
    //gtk_box_pack_start(GTK_BOX(hbox1), expenses_frame, TRUE, TRUE, 0);

    //vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    //g_object_set(vbox1, "margin-start", 10, "margin-end", 10, "margin-top", 10, NULL);
    //gtk_box_pack_start(GTK_BOX(vbox1), hbox1, TRUE, TRUE, 0);

    // Main window
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), expenses_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), sidebar, FALSE, FALSE, 0);
//    gtk_box_pack_start(GTK_BOX(main_vbox), statusbar, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(mainwin), main_vbox);
    gtk_widget_show_all(mainwin);

    g_signal_connect(mainwin, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_grab_focus(ctx->expenses_view);

    ctx->mainwin = mainwin;
}

int open_expense_file(uictx_t *ctx, char *xpfile) {
    FILE *f;
    char *filter;

    f = fopen(xpfile, "r");
    if (f == NULL)
        return 1;

    uictx_reset(ctx);

    load_expense_file(f, &ctx->all_xps);
    str_assign(ctx->xpfile, xpfile);
    fclose(f);

    //sort_expenses_by_date_desc(ctx->all_xps, ctx->all_xps_count);

    filter = (char *) gtk_entry_get_text(GTK_ENTRY(ctx->txt_filter));
    filter_expenses(&ctx->all_xps, &ctx->view_xps, filter, ctx->view_year, ctx->view_month);
    expensestv_refresh(GTK_TREE_VIEW(ctx->expenses_view), &ctx->view_xps, TRUE);

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
    col = gtk_tree_view_column_new_with_attributes("Date", r, "text", 0, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 0);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Description", r, "text", 1, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 1);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Amount", r, "text", 2, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 2);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_cell_renderer_set_alignment(r, 1.0, 0);
    gtk_tree_view_column_set_cell_data_func(col, r, expensestv_amt_datafunc, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Category", r, "text", 3, NULL);
    gtk_tree_view_column_set_sort_column_id(col, 3);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    ls = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING, G_TYPE_UINT);
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
    g_signal_connect(ts, "changed", G_CALLBACK(expensestv_row_changed), ctx);
    g_signal_connect(tv, "key-press-event", G_CALLBACK(expensestv_keypress), ctx);
    g_signal_connect(txt_filter, "changed", G_CALLBACK(expensestv_filter_changed), ctx);

    ctx->expenses_view = tv;
    ctx->txt_filter = txt_filter;
    return frame;
}

static void expensestv_amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data) {
    gdouble amt;
    char buf[15];

    gtk_tree_model_get(m, it, 2, &amt, -1);
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
    printf("expense_row_changed()\n");

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
    gchar *cat;
    guint rowid;

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(tv));
    gtk_tree_model_get(GTK_TREE_MODEL(ls), it, 0, &sdate, 1, &desc, 2, &amt, 3, &cat, 4, &rowid, -1);
    xp->dt = date_from_iso(sdate);
    str_assign(xp->time, "");
    str_assign(xp->desc, desc);
    str_assign(xp->cat, cat);
    xp->amt = amt;
    xp->rowid = rowid;

    g_free(sdate);
    g_free(desc);
    g_free(cat);
}

void expensestv_refresh(GtkTreeView *tv, array_t *xps, gboolean reset_cursor) {
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

    for (int i=0; i < xps->len; i++) {
        xp = xps->items[i];
        format_date_iso(xp->dt, isodate, sizeof(isodate));

        gtk_list_store_append(ls, &it);
        gtk_list_store_set(ls, &it, 
                           0, isodate,
                           1, xp->desc->s,
                           2, xp->amt,
                           3, xp->cat->s,
                           4, xp->rowid,
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

    cancel_wait_id(&ctx->view_wait_id);
    ctx->view_wait_id = g_timeout_add(200, expensestv_apply_filter, data);
}

static gboolean expensestv_apply_filter(gpointer data) {
    uictx_t *ctx = data;
    char *sfilter;

    sfilter = (char *) gtk_entry_get_text(GTK_ENTRY(ctx->txt_filter));
    filter_expenses(&ctx->all_xps, &ctx->view_xps, sfilter, ctx->view_year, ctx->view_month);
    expensestv_refresh(GTK_TREE_VIEW(ctx->expenses_view), &ctx->view_xps, FALSE);

    ctx->view_wait_id = 0;
    return G_SOURCE_REMOVE;
}

void expensestv_add_expense_row(uictx_t *ctx) {
    exp_t *xp;
    ExpenseEditDialog *d;
    gint z;
    int updated = 0;

    xp = exp_new();
    d = expeditdlg_new(xp);
    z = gtk_dialog_run(d->dlg);
    if (z == GTK_RESPONSE_OK) {
        expeditdlg_get_expense(d, xp);
        add_expense(&ctx->all_xps, xp);
        updated = 1;
    }
    expeditdlg_free(d);
    exp_free(xp);

    if (updated) {
        //apply_filter(ctx);
        //$$todo update ctx->cb_year if year was changed
    }
}

static void expensestv_edit_expense_row(GtkTreeView *tv, GtkTreeIter *it, uictx_t *ctx) {
    exp_t *xp;
    ExpenseEditDialog *d;
    gint z;
    uint rowid;
    int updated = 0;

    xp = exp_new();
    expensestv_get_expense(tv, it, xp);
    rowid = xp->rowid;

    d = expeditdlg_new(xp);
    z = gtk_dialog_run(d->dlg);
    if (z == GTK_RESPONSE_OK) {
        expeditdlg_get_expense(d, xp);
        assert(xp->rowid == rowid);

        update_expense(&ctx->all_xps, xp);
        updated = 1;
    }
    expeditdlg_free(d);
    exp_free(xp);

    if (updated) {
        //apply_filter(ctx);
        //$$todo update ctx->cb_year if year was changed
    }
}

static ExpenseEditDialog *expeditdlg_new(exp_t *xp) {
    ExpenseEditDialog *d;
    GtkWidget *dlg;
    GtkWidget *dlgbox;
    GtkWidget *lbl_date, *lbl_desc, *lbl_amt, *lbl_cat;
    GtkWidget *txt_date, *txt_desc, *txt_amt, *txt_cat;
    GtkWidget *tbl;
    char samt[12];
    char isodate[ISO_DATE_LEN+1];

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
    g_object_set(lbl_desc, "xalign", 0.0, NULL);
    g_object_set(lbl_amt, "xalign", 0.0, NULL);
    g_object_set(lbl_cat, "xalign", 0.0, NULL);

    txt_date = gtk_entry_new();
    txt_desc = gtk_entry_new();
    txt_amt = gtk_entry_new();
    txt_cat = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(txt_desc), 25);
    g_signal_connect(txt_amt, "insert-text", G_CALLBACK(expeditdlg_amt_insert_text_event), NULL);
    g_signal_connect(txt_date, "insert-text", G_CALLBACK(expeditdlg_date_insert_text_event), NULL);
    g_signal_connect(txt_date, "delete-text", G_CALLBACK(expeditdlg_date_delete_text_event), NULL);
    g_signal_connect(txt_date, "key-press-event", G_CALLBACK(expeditdlg_date_key_press_event), NULL);

    format_date_iso(xp->dt, isodate, sizeof(isodate));
    gtk_entry_set_text(GTK_ENTRY(txt_date), isodate); 
    gtk_entry_set_text(GTK_ENTRY(txt_desc), xp->desc->s); 
    snprintf(samt, sizeof(samt), "%.2f", xp->amt);
    gtk_entry_set_text(GTK_ENTRY(txt_amt), samt); 
    gtk_entry_set_text(GTK_ENTRY(txt_cat), xp->cat->s); 

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
    gtk_widget_show_all(dlg);

    d = malloc(sizeof(ExpenseEditDialog));
    d->dlg = GTK_DIALOG(dlg);
    d->txt_date = GTK_ENTRY(txt_date);
    d->txt_desc = GTK_ENTRY(txt_desc);
    d->txt_amt = GTK_ENTRY(txt_amt);
    d->txt_cat = GTK_ENTRY(txt_cat);
    return d;
}
static void expeditdlg_free(ExpenseEditDialog *d) {
    gtk_widget_destroy(GTK_WIDGET(d->dlg));
    free(d);
}

static void expeditdlg_get_expense(ExpenseEditDialog *d, exp_t *xp) {
    const gchar *sdate = gtk_entry_get_text(GTK_ENTRY(d->txt_date));
    const gchar *sdesc = gtk_entry_get_text(GTK_ENTRY(d->txt_desc));
    const gchar *samt = gtk_entry_get_text(GTK_ENTRY(d->txt_amt));
    const gchar *scat = gtk_entry_get_text(GTK_ENTRY(d->txt_cat));

    xp->dt = date_from_iso((char*) sdate);
    str_assign(xp->time, "");
    str_assign(xp->desc, (char*)sdesc);
    xp->amt = atof(samt);
    str_assign(xp->cat, (char*)scat);
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
static void expeditdlg_date_insert_text_event(GtkEntry* ed, gchar *newtxt, gint newtxt_len, gint *pos, gpointer data) {
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

static void copy_year_str(int year, char *syear, size_t syear_len) {
    if (year == 0)
        strncpy(syear, "All", syear_len);
    else
        snprintf(syear, syear_len, "%d", year);
}
static char *get_month_name(uint month) {
    static char *month_names[] = {"All", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

    assert(month < countof(month_names));
    return month_names[month];
}

static GtkWidget *create_year_menuitem(int year) {
    char syear[5];
    copy_year_str(year, syear, sizeof(syear));
    return gtk_menu_item_new_with_label(syear);
}
static GtkWidget *create_month_menuitem(int month) {
    return gtk_menu_item_new_with_label(get_month_name(month));
}

GtkWidget *create_sidebar_controls(uictx_t *ctx) {
    GtkWidget *frame;
    GtkWidget *hbox;
    GtkWidget *lbl;
    GtkWidget *menubtn;
    GtkWidget *menubtn_hbox;
    GtkWidget *icon;
    GtkWidget *menu;
    GtkWidget *mi;
    GtkWidget *addbtn, *editbtn, *delbtn;
    int years[] = {0, 2023, 2022, 2021, 2020, 2019, 2018, 2017, 2016, 2015};
    char syear[5];
    char smonth[20];
    int selyear = 2022;
    int selmonth = 12;

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    // Year selector menu
    menu = gtk_menu_new();
    for (int i=0; i < countof(years); i++) {
        mi = create_year_menuitem(years[i]);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }
    gtk_widget_show_all(menu);

    copy_year_str(selyear, syear, sizeof(syear));
    lbl = gtk_label_new(syear);
    icon = gtk_image_new_from_icon_name("pan-up-symbolic", GTK_ICON_SIZE_BUTTON);
    menubtn_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(menubtn_hbox), lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(menubtn_hbox), icon, FALSE, FALSE, 0);

    menubtn = gtk_menu_button_new();
    gtk_container_add(GTK_CONTAINER(menubtn), menubtn_hbox);
    gtk_menu_button_set_direction(GTK_MENU_BUTTON(menubtn), GTK_ARROW_UP);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(menubtn), menu);
    g_object_set(menu, "halign", GTK_ALIGN_END, NULL);

    gtk_box_pack_start(GTK_BOX(hbox), menubtn, FALSE, FALSE, 0);

    // Month selector menu
    menu = gtk_menu_new();
    for (int i=0; i <= 12; i++) {
        mi = create_month_menuitem(i);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    }
    gtk_widget_show_all(menu);

    lbl = gtk_label_new(get_month_name(selmonth));
    icon = gtk_image_new_from_icon_name("pan-up-symbolic", GTK_ICON_SIZE_BUTTON);
    menubtn_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(menubtn_hbox), lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(menubtn_hbox), icon, FALSE, FALSE, 0);

    menubtn = gtk_menu_button_new();
    gtk_container_add(GTK_CONTAINER(menubtn), menubtn_hbox);
    gtk_menu_button_set_direction(GTK_MENU_BUTTON(menubtn), GTK_ARROW_UP);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(menubtn), menu);
    g_object_set(menu, "halign", GTK_ALIGN_END, NULL);

    gtk_box_pack_start(GTK_BOX(hbox), menubtn, FALSE, FALSE, 0);

    addbtn = gtk_button_new_from_icon_name("list-add-symbolic", GTK_ICON_SIZE_BUTTON);
    editbtn = gtk_button_new_from_icon_name("document-edit-symbolic", GTK_ICON_SIZE_BUTTON);
    delbtn = gtk_button_new_from_icon_name("edit-delete-symbolic", GTK_ICON_SIZE_BUTTON);
    //addbtn = gtk_button_new_with_mnemonic("_Add");
    //editbtn = gtk_button_new_with_mnemonic("_Edit");
    //delbtn = gtk_button_new_with_mnemonic("_Delete");
    gtk_box_pack_end(GTK_BOX(hbox), delbtn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), editbtn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), addbtn, FALSE, FALSE, 0);

    return create_frame("", hbox, 4, 0);
}

