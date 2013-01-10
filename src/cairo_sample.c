#include <stdio.h>
#include <stdlib.h>
#include <cairo.h>
#include <cairo-gl.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <gdk/gdkx.h>

#include <unistd.h>
#include <pthread.h>

//the global pixmap that will serve as our buffer
static GdkPixmap *pixmap = NULL;

static gboolean rendering=FALSE;
//cairo_surface_t* window_surface=NULL;

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
create_source_surface(int width, int height)
{

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
	
	dpy = XOpenDisplay(NULL);
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
	

    surface = cairo_gl_surface_create (device,
									   CAIRO_CONTENT_COLOR_ALPHA,
									   width, height);
    cairo_device_destroy (device);
	
    return surface;

}


void do_draw(void *ptr)
{
    int width, height;

	rendering=TRUE;
	gdk_drawable_get_size(pixmap, &width, &height);
	//create a gtk-independant surface to draw on
    cairo_surface_t *cst = create_source_surface(width, height);
	//cairo_surface_t* cst = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(cst);
	
	//cr = cairo_create(window_surface);
	cairo_status_t status;
	status=cairo_status(cr);
	if(status)
		return;
	
	cairo_save(cr);
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);
	cairo_restore(cr);
	
	cairo_save(cr);
	cairo_set_source_rgb(cr, 0, .5, 0.2);
	cairo_set_line_width (cr, 0.5);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL);
//	cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OUT);
	for(int j=0;j<20;j++)
	{
		for(int i=0; i<100; i++)
		{
			cairo_line_to(cr, rand()%width, rand()%height);
		}
	}
	cairo_stroke(cr);
	cairo_restore(cr);
	cairo_destroy(cr);
	
	cairo_t *cr_pixmap = gdk_cairo_create(pixmap);
    cairo_set_source_surface (cr_pixmap, cst, 0, 0);
    cairo_paint(cr_pixmap);
    cairo_destroy(cr_pixmap);

	rendering=FALSE;

}

static gboolean
on_expose_event(GtkWidget *widget,
    GdkEventExpose *event,
    gpointer data)
{
	do_draw(NULL);
	
	gdk_draw_drawable(widget->window,
					  widget->style->fg_gc[GTK_WIDGET_STATE(widget)], pixmap,
					  // Only copy the area that was exposed.
					  event->area.x, event->area.y,
					  event->area.x, event->area.y,
					  event->area.width, event->area.height);

    return TRUE;
}

void on_destroy()
{
//	cairo_surface_destroy(window_surface);
	gtk_main_quit();
}


gboolean
on_configure_event(GtkWidget *w, GdkEventConfigure *event, gpointer data)
{
    static int oldw = 0;
    static int oldh = 0;
    //make our selves a properly sized pixmap if our window has been resized
    if (oldw != event->width || oldh != event->height){
        //create our new pixmap with the correct size.
        GdkPixmap *tmppixmap = gdk_pixmap_new(w->window, event->width,  event->height, -1);
        //copy the contents of the old pixmap to the new pixmap.  This keeps ugly uninitialized
        //pixmaps from being painted upon resize
        int minw = oldw, minh = oldh;
        if( event->width < minw ){ minw =  event->width; }
        if( event->height < minh ){ minh =  event->height; }
        gdk_draw_drawable(tmppixmap, w->style->fg_gc[GTK_WIDGET_STATE(w)], pixmap, 0, 0, 0, 0, minw, minh);
        //we're done with our old pixmap, so we can get rid of it and replace it with our properly-sized one.
        g_object_unref(pixmap);
        pixmap = tmppixmap;
    }
    oldw = event->width;
    oldh = event->height;
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

  pixmap = gdk_pixmap_new(window->window,400,300,-1);
  
  //window_surface=create_source_surface_for_widget(window);

  gtk_main();

  return 0;
}
