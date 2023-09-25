#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "bslib.h"
#include "expense.h"

void quit(const char *s);
void print_error(const char *s);
void panic(const char *s);
static void setup_ui(ExpContext *ctx);
static void refresh_filter_ui(ExpContext *ctx);
static GtkWidget *create_menubar(ExpContext *ctx, GtkWidget *mainwin);
static GtkWidget *create_xps_treeview();
static void amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data);
static int open_xpfile(ExpContext *ctx, char *xpfile);

static void file_open(GtkWidget *w, gpointer data);
static void cancel_wait_id(guint *wait_id);

static gboolean process_filter(gpointer data);
static void refresh_treeview_xps(GtkTreeView *tv, BSArray *xps, gboolean reset_cursor);
static void tp_to_it(GtkTreeView *tv, GtkTreePath *tp, GtkTreeIter *it);
static Expense *expense_from_treeiter(GtkTreeView *tv, GtkTreeIter *it);
static Expense *expense_from_treepath(GtkTreeView *tv, GtkTreePath *tp);

static void txt_filter_changed(GtkWidget *w, gpointer data);
static void cb_year_changed(GtkWidget *w, gpointer data);
static void cb_month_changed(GtkWidget *w, gpointer data);
static void tv_xps_rowactivated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer data);
static void ts_xps_changed(GtkTreeSelection *ts, gpointer data);

typedef struct {
    GtkDialog *dlg;
    GtkEntry *txt_date;
    GtkEntry *txt_desc;
    GtkEntry *txt_amt;
    GtkEntry *txt_cat;
    Expense *xp;
} XPEditDlg;

XPEditDlg *xpeditdlg_new(Expense *xp);
void xpeditdlg_free(XPEditDlg *xpdlg);

static char *month_names[] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

int main(int argc, char *argv[]) {
    ExpContext *ctx;
    int z;

    ctx = create_context();
    gtk_init(&argc, &argv);
    setup_ui(ctx);

    if (argc > 1) {
        z = open_xpfile(ctx, argv[1]);
        if (z != 0)
            print_error("Error reading expense file");
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

static void setup_ui(ExpContext *ctx) {
    GtkWidget *mainwin;
    GtkWidget *menubar;
    GtkWidget *statusbar;
    GtkWidget *notebook;
    GtkWidget *tv_xps;
    GtkWidget *sw_xps;
    GtkWidget *txt_filter;
    GtkWidget *cb_month;
    GtkWidget *cb_year;
    GtkWidget *vbox1;
    GtkWidget *hbox1;
    GtkTreeSelection *ts;

    // mainwin
    mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mainwin), "gtktest");
//    gtk_window_set_default_size(GTK_WINDOW(mainwin), 640, 480);
//    gtk_container_set_border_width(GTK_CONTAINER(mainwin), 10);
    gtk_widget_set_size_request(mainwin, 800, 600);

    menubar = create_menubar(ctx, mainwin);
    statusbar = gtk_statusbar_new();
    gtk_widget_set_halign(statusbar, GTK_ALIGN_END);

    guint statusid =  gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "info");
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), statusid, "Expense Buddy GUI");

    // Filter section
    hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    g_object_set(hbox1, "margin-start", 10, "margin-end", 10, NULL);

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

    gtk_box_pack_start(GTK_BOX(hbox1), txt_filter, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), cb_year, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), cb_month, FALSE, FALSE, 0);

    // Expenses list
    sw_xps = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw_xps), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    tv_xps = create_xps_treeview();
    gtk_container_add(GTK_CONTAINER(sw_xps), tv_xps);

    ts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv_xps));
    gtk_tree_selection_set_mode(ts, GTK_SELECTION_BROWSE);

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);
    g_object_set(notebook, "margin-start", 10, "margin-end", 10, NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw_xps, gtk_label_new("Expenses"));

    // Main window
    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox1), menubar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), notebook, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox1), statusbar, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(mainwin), vbox1);
    gtk_widget_show_all(mainwin);

    g_signal_connect(mainwin, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(txt_filter, "changed", G_CALLBACK(txt_filter_changed), ctx);
    g_signal_connect(cb_year, "changed", G_CALLBACK(cb_year_changed), ctx);
    g_signal_connect(cb_month, "changed", G_CALLBACK(cb_month_changed), ctx);
    g_signal_connect(tv_xps, "row-activated", G_CALLBACK(tv_xps_rowactivated), ctx);
    g_signal_connect(ts, "changed", G_CALLBACK(ts_xps_changed), ctx);

    gtk_widget_grab_focus(tv_xps);

    ctx->mainwin = mainwin;
    ctx->menubar = menubar;
    ctx->notebook = notebook;
    ctx->tv_xps = tv_xps;
    ctx->txt_filter = txt_filter;
    ctx->cb_year = cb_year;
    ctx->cb_month = cb_month;
}

static void refresh_filter_ui(ExpContext *ctx) {
    GtkComboBoxText *cb_year = GTK_COMBO_BOX_TEXT(ctx->cb_year);
    GtkComboBoxText *cb_month = GTK_COMBO_BOX_TEXT(ctx->cb_month);
    char syear[5];

    gtk_combo_box_text_remove_all(cb_year);
    gtk_combo_box_text_append_text(cb_year, "All");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cb_year), 0);
    for (int i=0; i < bs_countof(ctx->xps_years); i++) {
        uint year = ctx->xps_years[i];
        if (year == 0)
            break;
        snprintf(syear, sizeof(syear), "%d", year);
        gtk_combo_box_text_append_text(cb_year, syear);
    }

    gtk_combo_box_text_remove_all(cb_month);
    gtk_combo_box_text_append_text(cb_month, "All");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cb_month), 0);
    for (int i=1; i < bs_countof(month_names); i++) {
        gtk_combo_box_text_append_text(cb_month, month_names[i]);
    }
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

static GtkWidget *create_xps_treeview() {
    GtkWidget *tv;
    GtkCellRenderer *r;
    GtkTreeViewColumn *col;
    GtkListStore *ls;
    int xpadding = 10;
    int ypadding = 2;

    tv = gtk_tree_view_new();

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
    gtk_tree_view_column_set_cell_data_func(col, r, amt_datafunc, NULL, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Category", r, "text", 3, NULL);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_cell_renderer_set_padding(r, xpadding, ypadding);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tv), col);

    ls = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tv), GTK_TREE_MODEL(ls));
    g_object_unref(ls);

    return tv;
}

static void amt_datafunc(GtkTreeViewColumn *col, GtkCellRenderer *r, GtkTreeModel *m, GtkTreeIter *it, gpointer data) {
    gdouble amt;
    char buf[15];

    gtk_tree_model_get(m, it, 2, &amt, -1);
    snprintf(buf, sizeof(buf), "%9.2f", amt);
    g_object_set(r, "text", buf, NULL);
}

static void file_open(GtkWidget *w, gpointer data) {
    ExpContext *ctx = data;
    GtkWidget *dlg;
    gchar *xpfile;
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
    z = open_xpfile(ctx, xpfile);
    if (z != 0)
        print_error("Error reading expense file");

exit:
    if (xpfile != NULL)
        g_free(xpfile);
    gtk_widget_destroy(dlg);
}

static int open_xpfile(ExpContext *ctx, char *xpfile) {
    int z;

    z = load_xpfile(xpfile, ctx->xps);
    if (z != 0)
        return z;
    free(ctx->xpfile);
    ctx->xpfile = strdup(xpfile);

    sort_xps_bydate_desc(ctx->xps);
    get_xps_years(ctx->xps, ctx->xps_years, bs_countof(ctx->xps_years));
    get_xps_months(ctx->xps, ctx->xps_months, bs_countof(ctx->xps_months));
    refresh_filter_ui(ctx);

    filter_xps(ctx->xps, ctx->filtered_xps, "", 0, 0);
    refresh_treeview_xps(GTK_TREE_VIEW(ctx->tv_xps), ctx->filtered_xps, TRUE);

    return 0;
}

static void cb_year_changed(GtkWidget *w, gpointer data) {
    ExpContext *ctx = data;
    gchar *syear;
    int year = 0;

    syear = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(ctx->cb_year));
    if (syear == NULL)
        return;
    year = atoi(syear);
    if (year == ctx->filter_year)
        return;

    cancel_wait_id(&ctx->filter_wait_id);
    ctx->filter_year = year;
    process_filter(ctx);
}

static void cb_month_changed(GtkWidget *w, gpointer data) {
    ExpContext *ctx = data;
    int month = 0;

    month = gtk_combo_box_get_active(GTK_COMBO_BOX(ctx->cb_month));
    if (month == -1)
        return;
    if (month == ctx->filter_month)
        return;

    cancel_wait_id(&ctx->filter_wait_id);
    ctx->filter_month = month;
    process_filter(ctx);
}

static void txt_filter_changed(GtkWidget *w, gpointer data) {
    ExpContext *ctx = data;

    cancel_wait_id(&ctx->filter_wait_id);
    ctx->filter_wait_id = g_timeout_add(200, process_filter, data);
}

static void cancel_wait_id(guint *wait_id) {
    if (*wait_id != 0) {
        g_source_remove(*wait_id);
        *wait_id = 0;
    }
}

static gboolean process_filter(gpointer data) {
    ExpContext *ctx = data;
    const gchar *sfilter;

    sfilter = gtk_entry_get_text(GTK_ENTRY(ctx->txt_filter));
    printf("filter: '%s', year: %d, month: %d\n", sfilter, ctx->filter_year, ctx->filter_month);
    filter_xps(ctx->xps, ctx->filtered_xps, sfilter, ctx->filter_month, ctx->filter_year);
    refresh_treeview_xps(GTK_TREE_VIEW(ctx->tv_xps), ctx->filtered_xps, TRUE);

    ctx->filter_wait_id = 0;
    return G_SOURCE_REMOVE;
}

static void refresh_treeview_xps(GtkTreeView *tv, BSArray *xps, gboolean reset_cursor) {
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

    for (int i=0; i < xps->len; i++) {
        Expense *xp = bs_array_get(xps, i);
        gtk_list_store_append(ls, &it);
        gtk_list_store_set(ls, &it, 0, xp->dt->s, 1, xp->desc, 2, xp->amt, 3, xp->cat, -1);
    }

    gtk_tree_selection_set_mode(ts, GTK_SELECTION_BROWSE);

    if (reset_cursor) {
        tp = gtk_tree_path_new_from_string("0");
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(tv), tp, NULL, FALSE);
        gtk_tree_path_free(tp);
    }
}

static void tp_to_it(GtkTreeView *tv, GtkTreePath *tp, GtkTreeIter *it) {
    gtk_tree_model_get_iter(gtk_tree_view_get_model(tv), it, tp);
}

static Expense *expense_from_treeiter(GtkTreeView *tv, GtkTreeIter *it) {
    GtkListStore *ls;
    Expense *xp;
    gchar *sdate;
    gchar *desc;
    gdouble amt;
    gchar *cat;

    ls = GTK_LIST_STORE(gtk_tree_view_get_model(tv));
    gtk_tree_model_get(GTK_TREE_MODEL(ls), it, 0, &sdate, 1, &desc, 2, &amt, 3, &cat, -1);
    xp = expense_new(sdate, "", desc, amt, cat);

    g_free(sdate);
    g_free(desc);
    g_free(cat);
    return xp;
}

static Expense *expense_from_treepath(GtkTreeView *tv, GtkTreePath *tp) {
    GtkTreeIter it;
    tp_to_it(tv, tp, &it);
    return expense_from_treeiter(tv, &it);
}

static void tv_xps_rowactivated(GtkTreeView *tv, GtkTreePath *tp, GtkTreeViewColumn *col, gpointer data) {
    Expense *xp;
    XPEditDlg *xpdlg;
    gint z;

    printf("(rowactivated)\n");

    xp = expense_from_treepath(tv, tp);
    xpdlg = xpeditdlg_new(xp);
    z = gtk_dialog_run(xpdlg->dlg);

    xpeditdlg_free(xpdlg);
    expense_free(xp);
}

static void ts_xps_changed(GtkTreeSelection *ts, gpointer data) {
    GtkTreeView *tv;
    GtkListStore *ls;
    GtkTreeIter it;
    Expense *xp;

    if (!gtk_tree_selection_get_selected(ts, (GtkTreeModel **)&ls, &it))
        return;
    tv = gtk_tree_selection_get_tree_view(ts);
    xp = expense_from_treeiter(tv, &it);
    printf("(changed)\n");
    expense_free(xp);
}

void amt_insert_text(GtkEntry* ed, gchar *new_txt, gint len, gint *pos, gpointer data) {
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


XPEditDlg *xpeditdlg_new(Expense *xp) {
    XPEditDlg *xpdlg;
    GtkWidget *dlg;
    GtkWidget *dlgbox;
    GtkWidget *lbl_date, *lbl_desc, *lbl_amt, *lbl_cat;
    GtkWidget *txt_date, *txt_desc, *txt_amt, *txt_cat;
    GtkWidget *tbl;
    char samt[12];

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
    g_signal_connect(txt_amt, "insert-text", G_CALLBACK(amt_insert_text), NULL);

    gtk_entry_set_text(GTK_ENTRY(txt_date), xp->dt->s); 
    gtk_entry_set_text(GTK_ENTRY(txt_desc), xp->desc); 
    snprintf(samt, sizeof(samt), "%.2f", xp->amt);
    gtk_entry_set_text(GTK_ENTRY(txt_amt), samt); 
    gtk_entry_set_text(GTK_ENTRY(txt_cat), xp->cat); 

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

    xpdlg = malloc(sizeof(XPEditDlg));
    xpdlg->dlg = GTK_DIALOG(dlg);
    xpdlg->txt_date = GTK_ENTRY(txt_date);
    xpdlg->txt_desc = GTK_ENTRY(txt_desc);
    xpdlg->txt_amt = GTK_ENTRY(txt_amt);
    xpdlg->txt_cat = GTK_ENTRY(txt_cat);
    xpdlg->xp = xp;

    return xpdlg;
}

void xpeditdlg_free(XPEditDlg *xpdlg) {
    gtk_widget_destroy(GTK_WIDGET(xpdlg->dlg));
    free(xpdlg);
}

