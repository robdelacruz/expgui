#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>

#include "ui.h"

void set_screen_css(char *cssfile) {
    GdkScreen *screen = gdk_screen_get_default();
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, cssfile, NULL);
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

GtkWidget *create_scroll_window(GtkWidget *child) {
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(sw), child);

    return sw;
}

GtkWidget *create_frame(gchar *label, GtkWidget *child, int xpadding, int ypadding) {
    GtkWidget *fr = gtk_frame_new(label);
    if (xpadding > 0)
        g_object_set(child, "margin-start", xpadding, "margin-end", xpadding, NULL);
    if (ypadding > 0)
        g_object_set(child, "margin-top", xpadding, "margin-bottom", xpadding, NULL);
    gtk_container_add(GTK_CONTAINER(fr), child);
    return fr;
}

GtkWidget *create_label_with_margin(gchar *s, int start, int end, int top, int bottom) {
    GtkWidget *l = gtk_label_new(s);
    g_object_set(l, "margin-start", start, "margin-end", end, "margin-top", top, "margin-bottom", bottom, NULL);
    return l;
}

void add_accel(GtkWidget *w, GtkAccelGroup *a, guint key, GdkModifierType mods) {
    gtk_widget_add_accelerator(w, "activate", a, key, mods, GTK_ACCEL_VISIBLE);
}

GtkWidget *create_expenses_section(ExpContext *ctx) {
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
    gtk_tree_view_column_set_cell_data_func(col, r, currency_datafunc, NULL, NULL);
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
    //frame = create_frame("Expenses", vbox, 4, 0);
    frame = create_frame("", vbox, 4, 0);

    g_signal_connect(tv, "row-activated", G_CALLBACK(expense_row_activated), ctx);
    g_signal_connect(ts, "changed", G_CALLBACK(expense_row_changed), ctx);
    g_signal_connect(tv, "key-press-event", G_CALLBACK(expense_view_keypress), ctx);
    g_signal_connect(txt_filter, "changed", G_CALLBACK(txt_filter_changed), ctx);

    ctx->expenses_view = tv;
    ctx->txt_filter = txt_filter;
    return frame;
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
    GtkTreeSelection *ts;
    GtkTreeModel *m;
    GtkTreeIter it;

    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    if (!gtk_tree_selection_get_selected(ts, &m, &it))
        return;
    edit_expense_row(tv, &it, ctx);
}

static void expense_row_changed(GtkTreeSelection *ts, gpointer data) {
    ExpContext *ctx = data;
    GtkTreeView *tv;
    GtkListStore *ls;
    GtkTreeIter it;
    Expense xp;
    arena_t scratch = *ctx->scratch;
    printf("expense_row_changed()\n");

    if (!gtk_tree_selection_get_selected(ts, (GtkTreeModel **)&ls, &it))
        return;
    tv = gtk_tree_selection_get_tree_view(ts);
    //init_expense(&xp, &scratch);
    //get_expense_from_treeview(tv, &it, &xp);
}

static gboolean expense_view_keypress(GtkTreeView *tv, GdkEventKey *e, gpointer data) {
    ExpContext *ctx = data;
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
        edit_expense_row(tv, &it, ctx);
        return TRUE;
    } else if (kv == GDK_KEY_Delete || (e->state == GDK_CONTROL_MASK && kv == GDK_KEY_x)) {
    }

    return FALSE;
}

static void get_expense_from_treeview(GtkTreeView *tv, GtkTreeIter *it, Expense *xp) {
    GtkListStore *ls;
    gchar *sdate;
    gchar *desc;
    gdouble amt;
    gchar *cat;
    guint rowid;

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(tv));
    gtk_tree_model_get(GTK_TREE_MODEL(ls), it, 0, &sdate, 1, &desc, 2, &amt, 3, &cat, 4, &rowid, -1);
    xp->dt = date_from_iso(sdate);
    str_assign(&xp->time, "");
    str_assign(&xp->desc, desc);
    str_assign(&xp->cat, cat);
    xp->amt = amt;
    xp->rowid = rowid;

    g_free(sdate);
    g_free(desc);
    g_free(cat);
}

void refresh_expenses_treeview(GtkTreeView *tv, Expense *xps[], size_t xps_len, gboolean reset_cursor) {
    GtkListStore *ls;
    GtkTreeIter it;
    GtkTreePath *tp;
    GtkTreeSelection *ts;
    char isodate[ISO_DATE_LEN+1];
    Expense *xp;
    uint cur_rowid = 0;

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tv)));
    assert(ls != NULL);

    // (a) Remember active row to be restored later.
    gtk_tree_view_get_cursor(tv, &tp, NULL);
    if (tp) {
        gtk_tree_model_get_iter(GTK_TREE_MODEL(ls), &it, tp);
        gtk_tree_model_get(GTK_TREE_MODEL(ls), &it, 4, &cur_rowid, -1);
        printf("cur_rowid: %d\n", cur_rowid);
    }
    gtk_tree_path_free(tp);

    // Turn off selection while refreshing treeview so we don't get
    // bombarded by 'change' events.
    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
    gtk_tree_selection_set_mode(ts, GTK_SELECTION_NONE);

    gtk_list_store_clear(ls);

    for (int i=0; i < xps_len; i++) {
        xp = xps[i];
        format_date_iso(xp->dt, isodate, sizeof(isodate));

        gtk_list_store_append(ls, &it);
        gtk_list_store_set(ls, &it, 
                           0, isodate,
                           1, xp->desc.s,
                           2, xp->amt,
                           3, xp->cat.s,
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

static ExpenseEditDialog *create_expense_edit_dialog(arena_t *arena, Expense *xp) {
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

static void free_expense_edit_dialog(ExpenseEditDialog *d) {
    gtk_widget_destroy(GTK_WIDGET(d->dlg));
}

static void get_edit_expense(ExpenseEditDialog *d, Expense *xp) {
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

GtkWidget *create_sidebar_controls(ExpContext *ctx) {
    GtkWidget *frame;
    GtkWidget *vbox;
    GtkWidget *lbl;
    GtkWidget *cb_year;
    GtkWidget *cb_month;

    lbl = gtk_label_new("Time Period:");
    g_object_set(lbl, "xalign", 0.0,  NULL);
    cb_year = gtk_combo_box_text_new();
    cb_month = gtk_combo_box_text_new();

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), cb_year, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), cb_month, FALSE, FALSE, 0);

    g_signal_connect(cb_year, "changed", G_CALLBACK(cb_year_changed), ctx);
    g_signal_connect(cb_month, "changed", G_CALLBACK(cb_month_changed), ctx);

    ctx->cb_year = cb_year;
    ctx->cb_month = cb_month;
    return create_frame("", vbox, 4, 0);
}

void add_expense_row(ExpContext *ctx) {
    Expense xp;
    ExpenseEditDialog *d;
    gint z;
    arena_t scratch = *ctx->scratch;
    int updated = 0;

    init_expense(&xp, &scratch);
    d = create_expense_edit_dialog(&scratch, &xp);
    z = gtk_dialog_run(d->dlg);
    if (z == GTK_RESPONSE_OK) {
        get_edit_expense(d, &xp);
        add_expense(&xp, ctx);
        updated = 1;
    }
    free_expense_edit_dialog(d);

    if (updated) {
        apply_filter(ctx);
        //$$todo update ctx->cb_year if year was changed
    }
}

void edit_expense_row(GtkTreeView *tv, GtkTreeIter *it, ExpContext *ctx) {
    Expense xp;
    ExpenseEditDialog *d;
    gint z;
    arena_t scratch = *ctx->scratch;
    uint rowid;
    int updated = 0;

    init_expense(&xp, &scratch);
    get_expense_from_treeview(tv, it, &xp);
    rowid = xp.rowid;

    d = create_expense_edit_dialog(&scratch, &xp);
    z = gtk_dialog_run(d->dlg);
    if (z == GTK_RESPONSE_OK) {
        get_edit_expense(d, &xp);
        assert(xp.rowid == rowid);

        update_expense(&xp, ctx);
        updated = 1;
    }
    free_expense_edit_dialog(d);

    if (updated) {
        apply_filter(ctx);
        //$$todo update ctx->cb_year if year was changed
    }
}
