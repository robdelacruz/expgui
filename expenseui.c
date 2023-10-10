#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>

#include "expenseui.h"

static void currency_text_event(GtkEntry* ed, gchar *new_txt, gint len, gint *pos, gpointer data);
static void date_insert_text_event(GtkEntry* ed, gchar *new_txt, gint len, gint *pos, gpointer data);
static void date_delete_text_event(GtkEntry* ed, gint startpos, gint endpos, gpointer data);
static gboolean date_key_press_event(GtkEntry *ed, GdkEventKey *e, gpointer data);
static void currency_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data);

static void get_tree_it(GtkTreeView *tv, GtkTreePath *tp, GtkTreeIter *it);
static Expense *get_expense_from_treeview(arena_t *arena, GtkTreeView *tv, GtkTreeIter *it);

static void expense_row_activated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer data);
static void expense_row_changed(GtkTreeSelection *ts, gpointer data);

static gboolean apply_filter(gpointer data);
static void txt_filter_changed(GtkWidget *w, gpointer data);
static void cb_year_changed(GtkWidget *w, gpointer data);
static void cb_month_changed(GtkWidget *w, gpointer data);
static void cancel_wait_id(guint *wait_id);

static char *month_names[] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

GtkWidget *create_scroll_window(GtkWidget *child) {
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(sw), child);

    return sw;
}

GtkWidget *create_expenses_treeview(ExpContext *ctx) {
    GtkWidget *tv;
    GtkWidget *sw;
    GtkTreeSelection *ts;
    GtkCellRenderer *r;
    GtkTreeViewColumn *col;
    GtkListStore *ls;
    int xpadding = 10;
    int ypadding = 2;

    tv = gtk_tree_view_new();
    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    gtk_tree_selection_set_mode(ts, GTK_SELECTION_BROWSE);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Date", r, "text", 0, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Description", r, "text", 1, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_column_set_expand(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Amount", r, "text", 2, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_cell_renderer_set_alignment(r, 1.0, 0);
    gtk_tree_view_column_set_cell_data_func(col, r, currency_datafunc, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Category", r, "text", 3, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    ls = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ls));
    g_object_unref(ls);

    g_signal_connect(tv, "row-activated", G_CALLBACK(expense_row_activated), ctx);
    g_signal_connect(ts, "changed", G_CALLBACK(expense_row_changed), ctx);

    return tv;
}

static void currency_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data) {
    gdouble amt;
    char buf[15];

    gtk_tree_model_get(m, it, 2, &amt, -1);
    snprintf(buf, sizeof(buf), "%9.2f", amt);
    g_object_set(r, "text", buf, NULL);
}

static void expense_row_activated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer data) {
    ExpContext *ctx = data;
    GtkTreeIter it;
    Expense *xp;
    ExpenseEditDialog *d;
    gint z;
    arena_t scratch = *ctx->scratch;

    printf("(rowactivated)\n");

    get_tree_it(tv, tp, &it);
    xp = get_expense_from_treeview(&scratch, tv, &it);
    d = create_expense_edit_dialog(&scratch, scratch, xp);
    z = gtk_dialog_run(d->dlg);
    if (z == GTK_RESPONSE_OK) {
        char isodate[11];
        format_date_iso(xp->dt, isodate, sizeof(isodate));

        GtkListStore *ls = GTK_LIST_STORE(gtk_tree_view_get_model(tv));
        get_edit_expense(d, xp);
        gtk_list_store_set(ls, &it, 0, isodate, 1, xp->desc.s, 2, xp->amt, 3, xp->cat.s, -1);
    }
    free_expense_edit_dialog(d);
}

static void expense_row_changed(GtkTreeSelection *ts, gpointer data) {
    ExpContext *ctx = data;
    GtkTreeView *tv;
    GtkListStore *ls;
    GtkTreeIter it;
    Expense *xp;
    arena_t scratch = *ctx->scratch;

    if (!gtk_tree_selection_get_selected(ts, (GtkTreeModel **)&ls, &it))
        return;
    tv = gtk_tree_selection_get_tree_view(ts);
    xp = get_expense_from_treeview(&scratch, tv, &it);
    printf("(changed)\n");
}

static void get_tree_it(GtkTreeView *tv, GtkTreePath *tp, GtkTreeIter *it) {
    gtk_tree_model_get_iter(gtk_tree_view_get_model(tv), it, tp);
}

static Expense *get_expense_from_treeview(arena_t *arena, GtkTreeView *tv, GtkTreeIter *it) {
    GtkListStore *ls;
    Expense *xp;
    gchar *sdate;
    gchar *desc;
    gdouble amt;
    gchar *cat;

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(tv));
    gtk_tree_model_get(GTK_TREE_MODEL(ls), it, 0, &sdate, 1, &desc, 2, &amt, 3, &cat, -1);
    xp = create_expense(arena);
    xp->dt = date_from_iso(sdate);
    str_assign(&xp->time, "");
    str_assign(&xp->desc, desc);
    str_assign(&xp->cat, cat);
    xp->amt = amt;

    g_free(sdate);
    g_free(desc);
    g_free(cat);
    return xp;
}

void refresh_expenses_treeview(arena_t scratch, GtkTreeView *tv, Expense *xps[], size_t xps_len, gboolean reset_cursor) {
    GtkListStore *ls;
    GtkTreeIter it;
    GtkTreePath *tp;
    GtkTreeSelection *ts;

    // Turn off selection while refreshing treeview so we don't get
    // bombarded by 'change' events.
    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    gtk_tree_selection_set_mode(ts, GTK_SELECTION_NONE);

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
    assert(ls != NULL);

    gtk_list_store_clear(ls);

    for (int i=0; i < xps_len; i++) {
        char isodate[11];
        Expense *xp = xps[i];
        format_date_iso(xp->dt, isodate, sizeof(isodate));

        gtk_list_store_append(ls, &it);
        gtk_list_store_set(ls, &it, 0, isodate, 1, xp->desc.s, 2, xp->amt, 3, xp->cat.s, -1);
    }

    gtk_tree_selection_set_mode(ts, GTK_SELECTION_BROWSE);

    if (reset_cursor) {
        tp = gtk_tree_path_new_from_string("0");
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(tv), tp, NULL, FALSE);
        gtk_tree_path_free(tp);
    }
}

GtkWidget *create_filter_section(ExpContext *ctx) {
    GtkWidget *txt_filter;
    GtkWidget *cb_month;
    GtkWidget *cb_year;
    GtkWidget *hbox;

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    g_object_set(hbox, "margin-start", 10, "margin-end", 10, NULL);

    txt_filter = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(txt_filter), "Filter Expenses");

    cb_year = gtk_combo_box_text_new();
    g_object_set(cb_year, "margin-start", 5, "margin-end", 5, NULL);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_year), "All");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cb_year), 0);

    cb_month = gtk_combo_box_text_new();
    g_object_set(cb_month, "margin-start", 5, "margin-end", 5, NULL);
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb_month), "All");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cb_month), 0);

    gtk_box_pack_start(GTK_BOX(hbox), txt_filter, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), cb_year, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), cb_month, FALSE, FALSE, 0);

    g_signal_connect(txt_filter, "changed", G_CALLBACK(txt_filter_changed), ctx);
    g_signal_connect(cb_year, "changed", G_CALLBACK(cb_year_changed), ctx);
    g_signal_connect(cb_month, "changed", G_CALLBACK(cb_month_changed), ctx);

    ctx->txt_filter = txt_filter;
    ctx->cb_year = cb_year;
    ctx->cb_month = cb_month;

    return hbox;
}

void refresh_filter_ui(ExpContext *ctx) {
    GtkComboBoxText *cb_year = GTK_COMBO_BOX_TEXT(ctx->cb_year);
    GtkComboBoxText *cb_month = GTK_COMBO_BOX_TEXT(ctx->cb_month);
    char syear[5];

    gtk_combo_box_text_remove_all(cb_year);
    gtk_combo_box_text_append_text(cb_year, "All");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cb_year), 0);
    for (int i=0; i < countof(ctx->expenses_years); i++) {
        uint year = ctx->expenses_years[i];
        if (year == 0)
            break;
        snprintf(syear, sizeof(syear), "%d", year);
        gtk_combo_box_text_append_text(cb_year, syear);
    }

    gtk_combo_box_text_remove_all(cb_month);
    gtk_combo_box_text_append_text(cb_month, "All");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cb_month), 0);
    for (int i=1; i < countof(month_names); i++) {
        gtk_combo_box_text_append_text(cb_month, month_names[i]);
    }
}

static void cb_year_changed(GtkWidget *w, gpointer data) {
    ExpContext *ctx = data;
    gchar *syear;
    int year = 0;

    syear = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(ctx->cb_year));
    if (syear == NULL)
        return;
    year = atoi(syear);
    if (year == ctx->view_year)
        return;

    cancel_wait_id(&ctx->view_wait_id);
    ctx->view_year = year;
    apply_filter(ctx);
}

static void cb_month_changed(GtkWidget *w, gpointer data) {
    ExpContext *ctx = data;
    int month = 0;

    month = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->cb_month));
    if (month == -1)
        return;
    if (month == ctx->view_month)
        return;

    cancel_wait_id(&ctx->view_wait_id);
    ctx->view_month = month;
    apply_filter(ctx);
}

static void txt_filter_changed(GtkWidget *w, gpointer data) {
    ExpContext *ctx = data;

    cancel_wait_id(&ctx->view_wait_id);
    ctx->view_wait_id = g_timeout_add(200, apply_filter, data);
}

static void cancel_wait_id(guint *wait_id) {
    if (*wait_id != 0) {
        g_source_remove(*wait_id);
        *wait_id = 0;
    }
}

static gboolean apply_filter(gpointer data) {
    ExpContext *ctx = data;
    const gchar *sfilter;

    sfilter = gtk_entry_get_text(GTK_ENTRY(ctx->txt_filter));
    printf("filter: '%s', year: %d, month: %d\n", sfilter, ctx->view_year, ctx->view_month);
    filter_expenses(ctx->all_xps, ctx->all_xps_count,
                    ctx->view_xps, &ctx->view_xps_count,
                    sfilter, ctx->view_month, ctx->view_year);
    refresh_expenses_treeview(*ctx->scratch, GTK_TREE_VIEW(ctx->expenses_view), ctx->view_xps, ctx->view_xps_count, TRUE);

    ctx->view_wait_id = 0;
    return G_SOURCE_REMOVE;
}

ExpenseEditDialog *create_expense_edit_dialog(arena_t *arena, arena_t scratch, Expense *xp) {
    ExpenseEditDialog *d;
    GtkWidget *dlg;
    GtkWidget *dlgbox;
    GtkWidget *lbl_date, *lbl_desc, *lbl_amt, *lbl_cat;
    GtkWidget *txt_date, *txt_desc, *txt_amt, *txt_cat;
    GtkWidget *tbl;
    char samt[12];
    char isodate[11];

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
    g_signal_connect(txt_amt, "insert-text", G_CALLBACK(currency_text_event), NULL);
    g_signal_connect(txt_date, "insert-text", G_CALLBACK(date_insert_text_event), NULL);
    g_signal_connect(txt_date, "delete-text", G_CALLBACK(date_delete_text_event), NULL);
    g_signal_connect(txt_date, "key-press-event", G_CALLBACK(date_key_press_event), NULL);

    format_date_iso(xp->dt, isodate, sizeof(isodate));
    gtk_entry_set_text(GTK_ENTRY(txt_date), isodate); 
    gtk_entry_set_text(GTK_ENTRY(txt_desc), xp->desc.s); 
    snprintf(samt, sizeof(samt), "%.2f", xp->amt);
    gtk_entry_set_text(GTK_ENTRY(txt_amt), samt); 
    gtk_entry_set_text(GTK_ENTRY(txt_cat), xp->cat.s); 

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

    d = arena_alloc(arena, sizeof(ExpenseEditDialog));
    d->dlg = GTK_DIALOG(dlg);
    d->txt_date = GTK_ENTRY(txt_date);
    d->txt_desc = GTK_ENTRY(txt_desc);
    d->txt_amt = GTK_ENTRY(txt_amt);
    d->txt_cat = GTK_ENTRY(txt_cat);
    return d;
}

void free_expense_edit_dialog(ExpenseEditDialog *d) {
    gtk_widget_destroy(GTK_WIDGET(d->dlg));
}

void get_edit_expense(ExpenseEditDialog *d, Expense *xp) {
    const gchar *sdate = gtk_entry_get_text(GTK_ENTRY(d->txt_date));
    const gchar *sdesc = gtk_entry_get_text(GTK_ENTRY(d->txt_desc));
    const gchar *samt = gtk_entry_get_text(GTK_ENTRY(d->txt_amt));
    const gchar *scat = gtk_entry_get_text(GTK_ENTRY(d->txt_cat));

    xp->dt = date_from_iso((char*) sdate);
    str_assign(&xp->time, "");
    str_assign(&xp->desc, (char*)sdesc);
    xp->amt = atof(samt);
    str_assign(&xp->cat, (char*)scat);
}

static void currency_text_event(GtkEntry* ed, gchar *new_txt, gint len, gint *pos, gpointer data) {
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

// 2023-12-25
// 0123456789
// 1234567890

#define ISO_DATE_LEN 10

static void date_insert_text_event(GtkEntry* ed, gchar *newtxt, gint newtxt_len, gint *pos, gpointer data) {
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

    g_signal_handlers_block_by_func(G_OBJECT(ed), G_CALLBACK(date_insert_text_event), data);
    gtk_entry_set_text(ed, buf);
    g_signal_handlers_unblock_by_func(G_OBJECT(ed), G_CALLBACK(date_insert_text_event), data);
    g_signal_stop_emission_by_name(G_OBJECT(ed), "insert-text");
}

static void date_delete_text_event(GtkEntry* ed, gint startpos, gint endpos, gpointer data) {
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

static gboolean date_key_press_event(GtkEntry *ed, GdkEventKey *e, gpointer data) {
    if (e->keyval == GDK_KEY_Delete || e->keyval == GDK_KEY_KP_Delete)
        return TRUE;

    return FALSE;
}

