#include <stdio.h>
#include <cairo.h>
#include <gtk/gtk.h>


double coordx[100];
double coordy[100];

int count = 0;

static gboolean
on_expose_event(GtkWidget *widget,
    GdkEventExpose *event,
    gpointer data)
{
	cairo_t *cr;
	
	cr = gdk_cairo_create(widget->window);
	
	cairo_save(cr);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_set_line_width (cr, 0.5);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL);
	
	int i, j;
	for ( i = 0; i <= count - 1; i++ ) {
		for ( j  = 0; j <= count -1; j++ ) {
			cairo_move_to(cr, coordx[i], coordy[i]);
			cairo_line_to(cr, coordx[j], coordy[j]);
		}
	}
	cairo_stroke(cr);
	cairo_restore(cr);

	cairo_set_source_rgb(cr, 0.9, 0, 0);
	cairo_move_to(cr, 10, 10);
	GtkWindow *w=GTK_WINDOW(widget);
	gint x, y;
	gtk_window_get_position(w, &x, &y);
	char buff[60];
	sprintf(buff,"x=%d; y=%d", x, y);
	cairo_show_text(cr, buff);
	cairo_destroy(cr);

	if(count > 99) count=0;
	
	return FALSE;
}

gboolean clicked(GtkWidget *widget, GdkEventButton *event,
    gpointer user_data)
{
    if (event->button == 1) {
        coordx[count] = event->x;
        coordy[count++] = event->y;
    }

    if (event->button == 3) {
        count=0;
    }

    gtk_widget_queue_draw(widget);
    return TRUE;
}


int main (int argc, char *argv[])
{

  GtkWidget *window;

  gtk_init(&argc, &argv);
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
 
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

  g_signal_connect(window, "expose-event",
      G_CALLBACK(on_expose_event), NULL);
  g_signal_connect(window, "destroy",
      G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(window, "button-press-event", 
      G_CALLBACK(clicked), NULL);
 
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(window), "lines");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 300); 
  gtk_widget_set_app_paintable(window, TRUE);

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}
