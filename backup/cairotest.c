#include <cairo.h>
#include <gtk/gtk.h>

static void do_drawing(cairo_t *cr);
static gboolean draw_event(GtkWidget *w, cairo_t *cr, gpointer data);

int main(int argc, char *argv[]) {
    GtkWidget *w;
    GtkWidget *da;

    gtk_init(&argc, &argv);

    w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(w), "cairo test");
    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(w), 640, 480);

    da = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(w), da);

    g_signal_connect(G_OBJECT(da), "draw", G_CALLBACK(draw_event), NULL);
    g_signal_connect(w, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(w);
    gtk_main();

    return 0;
}

static void draw(cairo_t *cr) {
    cairo_set_source_rgb(cr, 0,0,0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 40.0);

    cairo_move_to(cr, 10.0, 50.0);
    cairo_show_text(cr, "Hello!");
}

static gboolean draw_event(GtkWidget *w, cairo_t *cr, gpointer data) {
    draw(cr);
    return FALSE;
}


