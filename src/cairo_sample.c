#include <stdio.h>
#include <stdlib.h>
#include <cairo.h>
#include <cairo-gl.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <gdk/gdkx.h>


static gboolean rendering=FALSE;
static cairo_surface_t* window_surface=NULL;

struct closure {
    Display *dpy;
    GLXContext ctx;
};


static void
cleanup (void *data)
{
    struct closure *arg = data;
	
    glXDestroyContext (arg->dpy, arg->ctx);
    XCloseDisplay (arg->dpy);
	
    free(arg);
}


static cairo_surface_t*
create_source_surface_for_widget(GtkWidget* widget)
{

	int width, height;
    int rgba_attribs[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_ALPHA_SIZE, 1,
		GLX_DOUBLEBUFFER,
		None };

    XVisualInfo *visinfo;
    GLXContext ctx;
    struct closure *arg;
    cairo_device_t *device;
    cairo_surface_t *surface;
    Display *dpy;
	
	dpy = GDK_WINDOW_XDISPLAY(gtk_widget_get_window(widget));
	
    if (dpy == NULL)
		return NULL;
    visinfo = glXChooseVisual (dpy, DefaultScreen (dpy), rgba_attribs);
    if (visinfo == NULL) {
		XCloseDisplay (dpy);
		return NULL;
    }
    ctx = glXCreateContext (dpy, visinfo, NULL, True);
    XFree (visinfo);
	
    if (ctx == NULL) {
		XCloseDisplay (dpy);
		return NULL;
    }
    arg = (struct closure*)(malloc (sizeof (struct closure)));
    arg->dpy = dpy;
    arg->ctx = ctx;
    device = cairo_glx_device_create (dpy, ctx);
    if (cairo_device_set_user_data (device,
									(cairo_user_data_key_t *) cleanup,
									arg,
									cleanup))
    {
		cleanup (arg);
		return NULL;
    }
	
	gtk_window_get_size(GTK_WINDOW(widget), &width, &height);

    surface = cairo_gl_surface_create_for_window (device,
									   GDK_WINDOW_XID(gtk_widget_get_window(widget)),
									   width, height);
    cairo_device_destroy (device);
	
    return surface;

}


static gboolean
on_expose_event(GtkWidget *widget,
    GdkEventExpose *event,
    gpointer data)
{
	if(rendering)
		return TRUE;
	rendering=TRUE;
	cairo_t *cr;

	cr = cairo_create(window_surface);
	cairo_status_t status;
	status=cairo_status(cr);
	if(status)
		return FALSE;
		
	cairo_save(cr);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);
	cairo_restore(cr);
	
	cairo_save(cr);
	cairo_set_source_rgb(cr, 0.9, 0, 0);
	cairo_rectangle(cr, 10, 10, 50, 80);
	cairo_fill(cr);
	cairo_restore(cr);
	
	cairo_save(cr);
	cairo_set_source_rgb(cr, 0, .5, 0.2);
	cairo_set_line_width (cr, 0.5);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_move_to(cr, 0, 0);
	cairo_line_to(cr, 110, 20);
	cairo_stroke(cr);
	cairo_restore(cr);
	
	
	/*Transfer drawings to screen*/
	cairo_gl_surface_swapbuffers (window_surface);
	
	cairo_destroy(cr);
	rendering=FALSE;
	
	return TRUE;
}

void on_destroy()
{
	gtk_main_quit();
}


gboolean
on_configure_event(GtkWidget *w, GdkEvent *event, gpointer data)
{
	gint x, y;
	x = event->configure.width;
	y = event->configure.height;
	cairo_gl_surface_set_size(window_surface, x, y);
	gtk_widget_queue_draw(w);
	return TRUE;
}

int main (int argc, char *argv[])
{

  GtkWidget *window;

  gtk_init(&argc, &argv);
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
 
  g_signal_connect(window, "expose-event",
      G_CALLBACK(on_expose_event), NULL);
  g_signal_connect(window, "destroy",
      G_CALLBACK(on_destroy), NULL);
  g_signal_connect(window, "configure-event",
	  G_CALLBACK(on_configure_event), NULL);


  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_title(GTK_WINDOW(window), "cairo_gl");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 300); 
  gtk_widget_set_app_paintable(window, TRUE);
  gtk_widget_set_double_buffered(window, FALSE);
  gtk_widget_show_all(window);

  window_surface=create_source_surface_for_widget(window);

  gtk_main();

  return 0;
}
