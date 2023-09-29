#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>

#include "expenseui.h"

static void amt_insert_text_event(GtkEntry* ed, gchar *new_txt, gint len, gint *pos, gpointer data);

ExpenseEditDialog *create_expense_edit_dialog(Expense *xp) {
    ExpenseEditDialog *d;
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
    g_signal_connect(txt_amt, "insert-text", G_CALLBACK(amt_insert_text_event), NULL);

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

    d = bs_malloc(sizeof(ExpenseEditDialog));
    d->dlg = GTK_DIALOG(dlg);
    d->txt_date = GTK_ENTRY(txt_date);
    d->txt_desc = GTK_ENTRY(txt_desc);
    d->txt_amt = GTK_ENTRY(txt_amt);
    d->txt_cat = GTK_ENTRY(txt_cat);
    return d;
}

void free_expense_edit_dialog(ExpenseEditDialog *d) {
    gtk_widget_destroy(GTK_WIDGET(d->dlg));
    bs_free(d);
}

void get_edit_expense(ExpenseEditDialog *d, Expense *xp) {
    const gchar *sdate = gtk_entry_get_text(GTK_ENTRY(d->txt_date));
    const gchar *sdesc = gtk_entry_get_text(GTK_ENTRY(d->txt_desc));
    const gchar *samt = gtk_entry_get_text(GTK_ENTRY(d->txt_amt));
    const gchar *scat = gtk_entry_get_text(GTK_ENTRY(d->txt_cat));

    bs_date_free(xp->dt);
    bs_free(xp->time);
    bs_free(xp->desc);
    bs_free(xp->cat);

    xp->dt = bs_date_iso_new((char*) sdate);
    xp->time = bs_strdup("");
    xp->desc = bs_strdup((char*) sdesc);
    xp->amt = atof(samt);
    xp->cat = bs_strdup((char*) scat);
}

static void amt_insert_text_event(GtkEntry* ed, gchar *new_txt, gint len, gint *pos, gpointer data) {
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


