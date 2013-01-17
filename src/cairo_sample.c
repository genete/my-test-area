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

#define CAIRO_SAMPLE_USE_GL

//the global pixmap that will serve as our buffer
static GdkPixmap *pixmap = NULL;


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
create_source_surface_for_size(int width, int height)
{

    int rgba_attribs[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_ALPHA_SIZE, 1,
		//GLX_SAMPLES, 4,
		//GLX_SAMPLE_BUFFERS, 1,
		GLX_STENCIL_SIZE, 1,
		GLX_DOUBLEBUFFER,
		None };

    XVisualInfo *visinfo;
    GLXContext ctx;
    struct closure *arg;
    cairo_device_t *device;
    cairo_surface_t *surface;
    Display *dpy;

	dpy = XOpenDisplay (NULL);

    if (dpy == NULL){
    		printf("Display is NULL\n");
		return NULL;
    }
    visinfo = glXChooseVisual (dpy, DefaultScreen (dpy), rgba_attribs);
    if (visinfo == NULL) {
    		printf("Visual Info is NULL\n");
		XCloseDisplay (dpy);
		return NULL;
    }
    ctx = glXCreateContext (dpy, visinfo, NULL, True);
    XFree (visinfo);

    if (ctx == NULL) {
    		printf("Glx Context is NULL\n");
		XCloseDisplay (dpy);
		return NULL;
    }
    arg = (struct closure*)(malloc (sizeof (struct closure)));
    arg->dpy = dpy;
    arg->ctx = ctx;
    device = cairo_glx_device_create (dpy, ctx);
    if (cairo_device_set_user_data (device, (cairo_user_data_key_t *) cleanup, arg, cleanup))
    {
    		printf("Can't set user data\n");
		cleanup (arg);
		return NULL;
    }

    surface = cairo_gl_surface_create (device, CAIRO_CONTENT_COLOR_ALPHA, width, height);
    cairo_device_destroy (device);

    return surface;

}


gboolean on_window_configure_event(GtkWidget * da, GdkEventConfigure * event, gpointer user_data){
    static int oldw = 0;
    static int oldh = 0;
    //make our selves a properly sized pixmap if our window has been resized
    if (oldw != event->width || oldh != event->height){
        //create our new pixmap with the correct size.
        GdkPixmap *tmppixmap = gdk_pixmap_new(da->window, event->width,  event->height, -1);
        //copy the contents of the old pixmap to the new pixmap.  This keeps ugly uninitialized
        //pixmaps from being painted upon resize
        int minw = oldw, minh = oldh;
        if( event->width < minw ){ minw =  event->width; }
        if( event->height < minh ){ minh =  event->height; }
        gdk_draw_drawable(tmppixmap, da->style->fg_gc[GTK_WIDGET_STATE(da)], pixmap, 0, 0, 0, 0, minw, minh);
        //we're done with our old pixmap, so we can get rid of it and replace it with our properly-sized one.
        g_object_unref(pixmap);
        pixmap = tmppixmap;
    }
    oldw = event->width;
    oldh = event->height;
    return TRUE;
}

gboolean on_window_expose_event(GtkWidget * da, GdkEventExpose * event, gpointer user_data){
    gdk_draw_drawable(da->window,
					  da->style->fg_gc[GTK_WIDGET_STATE(da)], pixmap,
					  // Only copy the area that was exposed.
					  event->area.x, event->area.y,
					  event->area.x, event->area.y,
					  event->area.width, event->area.height);
    return TRUE;
}


static int currently_drawing = 0;
//do_draw will be executed in a separate thread whenever we would like to update
//our animation
void *do_draw(void *ptr){

    currently_drawing = 1;

    int width, height;
    gdk_threads_enter();
    gdk_drawable_get_size(pixmap, &width, &height);
    gdk_threads_leave();

    //create a gtk-independant surface to draw on
#ifdef CAIRO_SAMPLE_USE_GL
    cairo_surface_t *cst = create_source_surface_for_size(width, height);
#else
    cairo_surface_t *cst = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
#endif
    if(cst==NULL) {
    	printf("surface is NULL\n");
    	return NULL;
    }
    cairo_t *cr = cairo_create(cst);
#ifdef CAIRO_SAMPLE_USE_GL
    // set thread aware to FALSE to reduce context switch
    cairo_gl_device_set_thread_aware (cairo_surface_get_device (cst), FALSE);
#endif
    //do some time-consuming drawing
    static int i = 0;
    ++i; i = i % 300;   //give a little movement to our animation
    cairo_set_source_rgb (cr, .9, .9, .9);
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


    //When dealing with gdkPixmap's, we need to make sure not to
    //access them from outside gtk_main().
    gdk_threads_enter();

    cairo_t *cr_pixmap = gdk_cairo_create(pixmap);
    cairo_set_source_surface (cr_pixmap, cst, 0, 0);
    cairo_paint(cr_pixmap);
    cairo_destroy(cr_pixmap);

#ifdef CAIRO_SAMPLE_USE_GL
    // set thread aware to true to allow context switch before we exit
    cairo_gl_device_set_thread_aware (cairo_surface_get_device (cst), TRUE);
#endif
    gdk_threads_leave();

    cairo_surface_destroy(cst);

    currently_drawing = 0;

    return NULL;
}

gboolean timer_exe(GtkWidget * window){

    static gboolean first_execution = TRUE;

    //use a safe function to get the value of currently_drawing so
    //we don't run into the usual multithreading issues
    int drawing_status = g_atomic_int_get(&currently_drawing);

    //if we are not currently drawing anything, launch a thread to
    //update our pixmap
    if(drawing_status == 0){
        static pthread_t thread_info;
        int  iret;
        if(first_execution != TRUE){
            pthread_join(thread_info, NULL);
        }
        iret = pthread_create( &thread_info, NULL, do_draw, NULL);
    }

    //tell our window it is time to draw our animation.
    int width, height;
    gdk_drawable_get_size(pixmap, &width, &height);
    gtk_widget_queue_draw_area(window, 0, 0, width, height);

    first_execution = FALSE;

    return TRUE;

}


int main (int argc, char *argv[]){


    //we need to initialize all these functions so that gtk knows
    //to be thread-aware
    //if (!g_thread_supported ()){ g_thread_init(NULL); }
    gdk_threads_init();
    gdk_threads_enter();

    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(window), "expose_event", G_CALLBACK(on_window_expose_event), NULL);
    g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(on_window_configure_event), NULL);

    //this must be done before we define our pixmap so that it can reference
    //the colour depth and such
    gtk_widget_show_all(window);

    //set up our pixmap so it is ready for drawing
    pixmap = gdk_pixmap_new(window->window,500,500,-1);
    //because we will be painting our pixmap manually during expose events
    //we can turn off gtk's automatic painting and double buffering routines.
    gtk_widget_set_app_paintable(window, TRUE);
    gtk_widget_set_double_buffered(window, FALSE);

    (void)g_timeout_add(33, (GSourceFunc)timer_exe, window);


    gtk_main();
    gdk_threads_leave();

    return 0;
}
