#include "../hdr/window_props.h"


#define MWM_HINTS_DECORATIONS (1L << 1)

// Motif hints structure
typedef struct
{
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
} MotifWmHints;

// Local function
static Bool set_window_property(Display *dpy, Window wdw, char *atomName, char stateF);  // add property to window
static void disable_window_decorations(Display *dpy, Window wnw);

static void disable_window_decorations(Display *dpy, Window wnw)
{
    Atom property = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
    MotifWmHints hints;
    hints.flags = MWM_HINTS_DECORATIONS;
    hints.decorations = 0;

    XChangeProperty(dpy, wnw, property, property, 32, PropModeReplace, (unsigned char *)&hints, 5);
}

void WP_window_cleanup(Display *dpy, Window wdw, GC gc)
{
    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, wdw);
    XCloseDisplay(dpy);
}

// Name         : WP_transparent_window
// Parameters   : dpy   > Display structure serves as the connection to the X server
// Call         : XDefaultRootWindow(), XMatchVisualInfo(), XDefaultScreen(), fprintf(),
//                exit(), XCreateColormap(), XCreateWindow()
// Called by    : main()
// Description  : Create transparent window
Window WP_transparent_window(Display* dpy, Window root, int screen)
{
    XVisualInfo vInfo;
    int depth = 32;    // Depth of the screen is 32 bits (4 bytes per pixel)
    unsigned long attrsMask = CWEventMask | CWBackPixel | CWBackPixmap | CWBorderPixel | CWColormap;
    XSetWindowAttributes attrs;
    Window wdw;

    if(!XMatchVisualInfo(dpy, screen, depth, TrueColor, &vInfo))
    {
        fprintf(stderr, "Error: Visual not found !\n");
        exit(-1);
    }

    // Window attributes
    attrs.background_pixel  = 0x40030313;       // transparent window
    attrs.background_pixmap = None;
    attrs.border_pixel      = 0;
    attrs.colormap          = XCreateColormap(dpy, root, vInfo.visual, AllocNone);
    attrs.event_mask        = ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    // Create window
    Window wnw = XCreateWindow(dpy, root, 0, 0, XDisplayWidth(dpy, screen), XDisplayHeight(dpy, screen), 0, depth, InputOutput,
                                vInfo.visual, attrsMask, &attrs);

    disable_window_decorations(dpy, wnw);
    XMapWindow(dpy, wnw);
    XFlush(dpy);
    return wnw;

}

Window WP_screenshot_window(Display *dpy, Window root, int screen)
{
    XSetWindowAttributes attr;
    attr.background_pixel = 0;      // black background
    attr.event_mask = ExposureMask | StructureNotifyMask;


    Window wnw = XCreateWindow(dpy, root, 0, 0, XDisplayWidth(dpy, screen), XDisplayHeight(dpy, screen), 0,
                               CopyFromParent, InputOutput, CopyFromParent, CWBackPixel | CWEventMask, &attr);

    disable_window_decorations(dpy, wnw);
    XMapWindow(dpy, wnw);
    XFlush(dpy);
    return wnw;
}

void WP_skip_window_border(Display *display, Window win) {

    Atom wm_state = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom wm_type_dock = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);

    XChangeProperty(display, win, wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&wm_type_dock, 1);
    XFlush(display);
}

// Name         : set_window_property
// Parameters   : dpy       > Display structure serves as the connection to the X server
//                wdw       > Window ID
//                atomName  > property atom to add
// Call         : XInternAtom(), fprintf(), memset(), XSendEvent(),
//                XDefaultRootWindow(), XFlush()
// Called by    : WP_skip_taskbar(), WP_full_screen(), WP_keep_above()
// Description  : add property to window
Bool set_window_property(Display *dpy, Window wdw, char *atomName, char stateF)
{

    Atom propertyAtom = XInternAtom(dpy, atomName, True);
    if(propertyAtom == None)
    {
        fprintf(stderr, "ERROR: %s not found!\n", atomName );
        return False;
    }

    Atom wmState = XInternAtom(dpy, "_NET_WM_STATE", True);
    if(wmState == None)
    {
        fprintf(stderr, "ERROR:  _NET_WM_STATE not found!\n" );
        return False;
    }

    XClientMessageEvent mEv;
    memset(&mEv, 0, sizeof(mEv));
    mEv.type = ClientMessage;
    mEv.window = wdw;
    mEv.message_type = wmState;
    mEv.format = 32;
    mEv.data.l[0] = stateF;
    mEv.data.l[1] = propertyAtom;
    mEv.data.l[2] = 0;
    mEv.data.l[3] = 0;
    mEv.data.l[4] = 0;
    XSendEvent(dpy, XDefaultRootWindow(dpy), False,
          SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *)&mEv );
    XFlush(dpy);
    return True;
}


// Name         : WP_skip_taskbar
// Parameters   : dpy   > Display structure serves as the connection to the X server
//                wdw   > Window ID
// Call         : set_window_property()
// Called by    : main()
// Description  : Window not appear in taskbar
Bool WP_skip_taskbar(Display *dpy, Window wdw, char stateF)
{
    return set_window_property(dpy, wdw, "_NET_WM_STATE_SKIP_TASKBAR", stateF);
}

// Name         : WP_keep_above
// Parameters   : dpy   > Display structure serves as the connection to the X server
//                wdw   llows sharing of code between different source files, promoting modularity and reusability. The> Window ID
// Call         : set_window_property()
// Called by    : main(), CP_color_picker()
// Description  : Keep window above other windows

Bool WP_keep_above(Display* dpy, Window wdw, char stateF)
{
    return set_window_property(dpy, wdw, "_NET_WM_STATE_ABOVE", stateF);
}

// Name         : WP_full_screen
// Parameters   : dpy   > Display structure serves as the connection to the X server
//                wdw   > Window ID
// Call         : set_window_property()
// Called by    : main()
// Description  : Full screen window
Bool WP_full_screen(Display* dpy, Window wdw, char stateF)
{
    return set_window_property(dpy, wdw, "_NET_WM_STATE_FULLSCREEN", stateF);
}

