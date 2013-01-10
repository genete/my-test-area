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


static gboolean rendering=FALSE;
cairo_surface_t* window_surface=NULL;

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


void *do_draw(void *ptr){
	
    rendering=TRUE;
	
    cairo_t *cr = cairo_create(window_surface);
	
	if(cairo_status(cr))
		return NULL;
	
    //do some time-consuming drawing
    static int i = 0;
    ++i; i = i % 300;   //give a little movement to our animation
    cairo_set_source_rgb (cr, .9, .2, .9);
    cairo_paint(cr);
    int j,k;
    for(k=0; k<100; ++k){   //lets just redraw lots of times to use a lot of proc power
        for(j=0; j < 1000; ++j){
            cairo_set_source_rgb (cr, (double)j/1000.0, (double)j/1000.0, 1.0 - (double)j/1000.0);
            cairo_move_to(cr, i,j/2);
            cairo_line_to(cr, i+100,j/2);
            cairo_stroke(cr);
        }
    }
    cairo_destroy(cr);

	cairo_gl_surface_swapbuffers (window_surface);

	rendering=FALSE;

    return NULL;
}


gboolean timer_exe(GtkWidget * window){
	
    static gboolean first_execution = TRUE;
	
    //use a safe function to get the value of currently_drawing so
    //we don't run into the usual multithreading issues
    gboolean drawing_status = g_atomic_int_get(&rendering);
	
    //if we are not currently drawing anything, launch a thread to
    //update our pixmap
    if(drawing_status == 0){
        static pthread_t thread_info;
        if(first_execution != TRUE){
            pthread_join(thread_info, NULL);
        }
        pthread_create( &thread_info, NULL, do_draw, NULL);
    }

	first_execution = FALSE;
	
    return TRUE;
	
}



int main (int argc, char *argv[])
{

    gdk_threads_init();
    gdk_threads_enter();

	
	gtk_init(&argc, &argv);

	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	
	gtk_window_set_title(GTK_WINDOW(window), "cairo_gl");
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
	gtk_widget_set_app_paintable(window, TRUE);
	gtk_widget_set_double_buffered(window, FALSE);
	
	gtk_widget_show_all(window);
	
	window_surface=create_source_surface_for_widget(window);
	
	//(void)g_timeout_add(33, (GSourceFunc)timer_exe, window);
	
	do_draw(NULL);
	
	gtk_main();
	
	gdk_threads_leave();
	return 0;
}
