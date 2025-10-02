#ifndef WINDOW_PROPS_H_
#define WINDOW_PROPS_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/extensions/Xfixes.h>
#include<X11/Xatom.h>

#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_REMOVE        0    /* remove property */

Window WP_transparent_window(Display* dpy, Window root, int screen);
Window WP_screenshot_window(Display *dpy, Window root, int screen);
Bool WP_keep_above(Display *dpy, Window wdw, char stateF);    // Keep window above
Bool WP_full_screen(Display *dpy, Window wdw, char statF);    // Full screen window
Bool WP_skip_taskbar(Display *dpy, Window wdw, char stateF);  // Window not appear in taskbar
void WP_skip_window_border(Display *display, Window win);
void WP_window_cleanup(Display *dpy, Window wdw, GC gc);

#endif
