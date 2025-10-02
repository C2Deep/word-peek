// Compile:
//          gcc -o main main.c src_scroll_win.c src_windowprops.c src_snipptshot.c src_img2txt.c src_curl.c `pkg-config --cflags --libs x11 xft`  -lpng         -ltesseract -llept  -lcurl -lcjson

#include "../hdr/window_props.h"
#include "../hdr/snipptshot.h"
#include "../hdr/img2txt.h"
#include "../hdr/curl_word_defs.h"
#include "../hdr/dictionary_window.h"
#include "../hdr/set_abs_path.h"

int main(void)
{
    Display *dpy;
    Window root, tWnw, sWnw;            // sWnw = screenshot window
    GC gc;
    XGCValues gcVals;
    XEvent event;
    XImage *ssImg, *gsImg;

    int screen, screenW, screenH,
        startX, startY, currentX, currentY,
        rectX, rectY, rectW, rectH;

    char shotF = 0, exitF = 0, drawF = 0,         // snipptshot Flag
        *imageName = "processedWord.png",
        imgText[128] = {0};

    // Set the process working directory to be where the program saved
    set_process_abs_path();

    if (!(dpy = XOpenDisplay(NULL)))
    {
        fprintf(stderr, "Cannot open dpy\n");
        return -1;
    }

    root   = XRootWindow(dpy, XDefaultScreen(dpy));
    screen = XDefaultScreen(dpy);
    screenW = XDisplayWidth(dpy, screen);
    screenH = XDisplayHeight(dpy, screen);

    ssImg = SS_get_window_screenshot(dpy, root);
    gsImg = SS_grayscale_img(dpy, root, ssImg);
    XDestroyImage(ssImg);

    sWnw = WP_screenshot_window(dpy, root, screen);
    WP_skip_taskbar(dpy, sWnw, _NET_WM_STATE_ADD);
    SS_put_screenshot_img(dpy, sWnw, gsImg);

    // Create transparent window
    tWnw = WP_transparent_window(dpy, root, screen);
    WP_keep_above(dpy, tWnw, _NET_WM_STATE_ADD);
    WP_skip_taskbar(dpy, tWnw, _NET_WM_STATE_ADD);

    gcVals.foreground = 0xffffff00;
    gcVals.line_width = 2;

    gc = XCreateGC(dpy, tWnw, GCForeground | GCLineWidth, &gcVals);

     while (!exitF)
     {
         XNextEvent(dpy, &event);

        switch (event.type) {
            case Expose:
                break;

            case ButtonPress:

                if (event.xbutton.button == Button1) {

                    // Clear previous rectangle (by redrawing the window background)
                    XClearWindow(dpy, tWnw);

                    startX = event.xbutton.x;
                    startY = event.xbutton.y;
                    drawF = 1;
                }

                    break;

            case MotionNotify:
                if (drawF) {

                    shotF = 1;

                    // Clear previous rectangle (by redrawing the window background)
                    XClearWindow(dpy, tWnw);

                    currentX = event.xmotion.x;
                    currentY = event.xmotion.y;

                    rectX = (startX < currentX) ? startX : currentX;
                    rectY = (startY < currentY) ? startY : currentY;
                    rectW = abs(currentX - startX);
                    rectH = abs(currentY - startY);

                    XDrawRectangle(dpy, tWnw, gc, rectX, rectY, rectW, rectH);
                }
                break;

            case ButtonRelease:
                char img2processF = 0;
                if (event.xbutton.button == Button1)
                {
                     drawF = 0;

                    if(shotF)
                    {
                        SS_snippt_shot(dpy, root,  rectX + (gcVals.line_width / 2), rectY + (gcVals.line_width / 2),
                                    rectW - (gcVals.line_width), rectH - (gcVals.line_width));
                        // we done
                        shotF = 0;
                        img2processF = 1;
                    }
                    else
                        exitF = 1;
                }

                else if(event.xbutton.button == Button3)
                {
                    XClearWindow(dpy, tWnw);
                    unsigned int wordPos[4] = {0};
                    startX = event.xbutton.x;
                    startY = event.xbutton.y;

                    char awsF = SS_auto_word_selection(dpy, gsImg, startX, startY, wordPos);

                    if(awsF)
                    {
                        img2processF = 1;

                        XDrawRectangle(dpy, tWnw, gc, wordPos[0] - (gcVals.line_width/2), wordPos[1] - (gcVals.line_width/2), wordPos[2] + gcVals.line_width, wordPos[3] + gcVals.line_width);
                        DW_redraw_window(dpy, tWnw);    // Force redraw
                    }
                }

                if(img2processF)
                {
                    image_2_text(imageName, "eng", imgText);

                    char *dictionaryOutput = CURL_word_definitions(imgText);

                    WP_keep_above(dpy, tWnw, _NET_WM_STATE_REMOVE);
                    WP_keep_above(dpy, sWnw, _NET_WM_STATE_REMOVE);
                    DW_dictionary_window(dpy, dictionaryOutput);
                    WP_keep_above(dpy, sWnw, _NET_WM_STATE_ADD);
                    WP_keep_above(dpy, tWnw, _NET_WM_STATE_ADD);
                    free(dictionaryOutput);
                    XClearWindow(dpy, tWnw); // clear the selecting rectangle
                    remove(imageName);
                }
                break;
        }
     }

    WP_window_cleanup(dpy, tWnw, gc);
    XDestroyImage(gsImg);
    return 0;
}
