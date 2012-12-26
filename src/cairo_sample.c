#include <stdio.h>
#include <cairo.h>
#include <cairo-gl.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <gdk/gdkx.h>


double coordx[100];
double coordy[100];

int count = 0;
char buff[60];

gboolean rendering=FALSE;
static cairo_surface_t* window_surface=NULL;

static cairo_device_t *
cairo_sample_gl_context_create(Display *dpy, GLXContext *out_gl_ctx)
{
    int attribs[] = { GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 1,
		None };
    GLXContext gl_ctx;
    XVisualInfo *visinfo;
    cairo_device_t *ctx;
	
    visinfo = glXChooseVisual (dpy, DefaultScreen (dpy), attribs);
    if (visinfo == NULL) {
		fprintf(stderr, "Failed to create RGB, double-buffered visual\n");
		return NULL;
    }
	
    gl_ctx = glXCreateContext (dpy, visinfo, NULL, True);
    *out_gl_ctx = gl_ctx;
	
    XFree(visinfo);
	
    ctx = cairo_glx_device_create (dpy, gl_ctx);
	
    return ctx;
}


static gboolean
on_expose_event(GtkWidget *widget,
    GdkEventExpose *event,
    gpointer data)
{
	if(rendering)
		return TRUE;
	rendering=TRUE;
	gint width, height;
	cairo_t *cr;
	GLXContext gl_ctx;
	cairo_device_t *ctx = NULL;
	/* Create a cairo_device for the Display and gl context*/
	ctx=cairo_sample_gl_context_create(GDK_WINDOW_XDISPLAY(gtk_widget_get_window(widget)), &gl_ctx);
	/*Create a surface for the window*/
	gtk_window_get_size(GTK_WINDOW(widget), &width, &height);
	window_surface=cairo_gl_surface_create_for_window(ctx, GDK_WINDOW_XID(gtk_widget_get_window(widget)), width, height);
	/*Create the cairo context*/
	cr = cairo_create(window_surface);
	cairo_status_t status;
	status=cairo_surface_status(window_surface);
	/*Drawing stuff*/
	cairo_save(cr);
	cairo_reset_clip(cr);
	cairo_set_source_rgb(cr, 0.4, 0.8, 0.8);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
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
	cairo_show_text(cr, buff);
	cairo_destroy(cr);
	
	/*Transfer drawings to screen*/
	cairo_gl_surface_swapbuffers (window_surface);
	
	cairo_device_destroy(ctx);
	cairo_surface_destroy(window_surface);
	if(count > 99) count=0;
	
	rendering=FALSE;
	
	return FALSE;
}


gboolean
on_configure_event(GtkWidget *w,
						GdkEvent *event, gpointer data)
{
	gint x, y;
	x = event->configure.x;
	y = event->configure.y;
	sprintf(buff,"x=%d; y=%d", x, y);
	gtk_widget_queue_draw(w);
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
 
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK|GDK_CONFIGURE);

  g_signal_connect(window, "expose-event",
      G_CALLBACK(on_expose_event), NULL);
  g_signal_connect(window, "destroy",
      G_CALLBACK(gtk_main_quit), NULL);
  g_signal_connect(window, "button-press-event", 
      G_CALLBACK(clicked), NULL);
  g_signal_connect(window, "configure-event",
	  G_CALLBACK(on_configure_event), NULL);

  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(window), "linese");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 300); 
  gtk_widget_set_app_paintable(window, TRUE);
  gtk_widget_set_double_buffered(window, FALSE);
  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}
