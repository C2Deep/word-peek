#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlocale.h>
#include <X11/keysym.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../hdr/window_props.h"

#define WINDOW_TOTAL_LINES_NUM      20
#define WINDOW_WIDTH_HEIGHT_RATIO   1.62

#define HORIZ_OFFSET        15

// Global functions
int DW_dictionary_window(Display *dpy, char *dicDefs);
void DW_redraw_window(Display *dpy, Window wnw);

// Local functions
static Window create_window(Display *dpy, unsigned int windowWidth, unsigned int windowHeight, Atom *wmDelete);
static unsigned int get_str_px_width(Display *display, XftFont *font, char *str);
static void free_dictionary_definitons(char **dicDefs);
static char *str_split_line(Display* dpy, XftFont *font, char *line, int linePxLimit);
static char **window_width_lines(Display* dpy, XftFont *font, int wPxSize, char *dicDefs, unsigned int *totalLines);
static XftFont *resize_font(Display *dpy, int screen, XftFont *font, unsigned int fontSize);
static void resize_window(Display *dpy, int screen, Window wnw, XftFont *font, unsigned int *wnwW, unsigned int *wnwH);

static Window create_window(Display *dpy, unsigned int wnwW, unsigned int wnwH, Atom *wmDelete)
{
    int screen = XDefaultScreen(dpy);
    Window root = XRootWindow(dpy, screen);

    int screenW = XDisplayWidth(dpy, screen);
    int screenH = XDisplayHeight(dpy, screen);

    int x = (screenW - wnwW) / 2;
    int y = (screenH - wnwH) / 2;
    // Create window
    Window wnw = XCreateSimpleWindow(dpy, root, x, y, wnwW, wnwH, 1,
                                     BlackPixel(dpy, screen), 0x00220022);
    // Window title
    XStoreName(dpy, wnw, "Dictionary definitions");

    *wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, wnw, wmDelete, 1);

    XSelectInput(dpy, wnw, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask);

    XMapWindow(dpy, wnw);
    return wnw;
}
void DW_redraw_window(Display *dpy, Window wnw)
{
    XExposeEvent expose;
    memset(&expose, 0, sizeof(expose));
    expose.type = Expose;
    expose.display = dpy;
    expose.window = wnw;
    expose.send_event = True;
    XSendEvent(dpy, wnw, False, ExposureMask, (XEvent *)&expose);
}

static unsigned int get_str_px_width(Display *dpy, XftFont *font, char *str)
{
     // Measure text width in pixels
    static XGlyphInfo extents;
    XftTextExtentsUtf8(dpy, font, (FcChar8 *)str, strlen(str), &extents);
    return extents.xOff;
}

static char *str_split_line(Display* dpy, XftFont *font, char *line, int linePxLimit)
{
    static char *lineBuf = NULL; // Line buffer
    char buf[1024] = {0};
    static char *sPtr = NULL;
    char *wwLine = NULL;
    static char exitF = 0;

    if(exitF)
    {
        exitF = 0;
        return NULL;
    }

    if(line)
    {
        if(!(lineBuf = malloc(strlen(line) + 1)))
        {
            fprintf(stderr, "Couldn't allocate memory.\n");
            exit(-1);
        }

        strcpy(lineBuf, line);
        sPtr = strtok(lineBuf, " ");
    }

    for(int pxSize = 0 ; sPtr ; )
    {
        if((pxSize = get_str_px_width(dpy, font, buf) + get_str_px_width(dpy, font, sPtr) + 1) < linePxLimit)
         {
             strcat(buf, sPtr);
             strcat(buf, " ");
         }
         else
             break;

        sPtr = strtok(NULL, " ");
    }

    if(!sPtr)
    {
        free(lineBuf);
        lineBuf = NULL;
        exitF = 1;
    }

    if(!(wwLine = malloc(strlen(buf) + 1)))
    {
        fprintf(stderr, "Couldn't allocate memory.\n");
        exit(-1);
    }

    strcpy(wwLine, buf);

    return wwLine;
}

static char **window_width_lines(Display* dpy, XftFont *font, int wPxSize, char *dicDefs, unsigned int *totalLines)
{
    int i = 0;
    char **wwDicDefs = NULL;
    char *dicLine;
    char *savePtr = NULL;

    char *dicDefsClone;

    if(!(dicDefsClone = malloc(strlen(dicDefs) + 1)))
    {
        fprintf(stderr, "Couldn't allocate memory.\n");
        exit(-1);
    }

    strcpy(dicDefsClone, dicDefs);

    dicLine = strtok_r(dicDefsClone, "\n", &savePtr);

    for( ; dicLine ;  ++i)
    {

        if(i == 0)
        {
            if(!(wwDicDefs = malloc(2 * sizeof(char *))))
            {
                fprintf(stderr, "Couldn't allocate memory.\n");
                exit(-1);
            }
        }
        else
        {
            if(!(wwDicDefs = realloc(wwDicDefs, (i + 2) * sizeof(char *))))
            {
                fprintf(stderr, "Couldn't allocate memory.\n");
                exit(-1);
            }
        }

        if(get_str_px_width(dpy, font, dicLine) < wPxSize)
        {
            if(!(wwDicDefs[i] = malloc(strlen(dicLine) + 1)))
            {
                fprintf(stderr, "Couldn't allocate memory of %ld bytes.\n", strlen(dicLine));
                exit(-1);
            }
            strcpy(wwDicDefs[i], dicLine);
        }

        else
        {
            wwDicDefs[i]  = str_split_line(dpy, font, dicLine, wPxSize);

            char *wwLine = NULL;

            ++i;
            while( wwLine = str_split_line(dpy, font, NULL, wPxSize) )
            {
                if(!(wwDicDefs = realloc(wwDicDefs, (i + 2) * sizeof(char *))))
                {
                    fprintf(stderr, "Couldn't allocate memory.\n");
                    exit(-1);
                }

                wwDicDefs[i] = wwLine;
                ++i;
            }
           --i;
        }
        dicLine = strtok_r(NULL, "\n", &savePtr);
    }

    free(dicDefsClone);
    wwDicDefs[i] = NULL;

    *totalLines = i;
    return wwDicDefs;
}

int DW_dictionary_window(Display *dpy, char *dicDefs)
{
    unsigned int wnwW, wnwH;
    int scroll_offset = 0;
    unsigned char margin = 2*HORIZ_OFFSET;
    int maxScrollOffset;

    int screen;
    Window wnw;

    XftColor bgClr, fgClr;
    XftFont *font;
    XRenderColor rndrClr;
    XftDraw *draw;
    XEvent ev;
    KeySym keySym;

    char **wwDicDefs;
    unsigned int wnwTotalLines = 0,
                 totalLines = 0,
                 fontSize = 17;

    char fontPattern[64] = "DejaVu Sans:pixelsize=17";
    int fontHeight = 0;

    screen = XDefaultScreen(dpy);

    font = XftFontOpenName(dpy, screen, fontPattern); // You can change the font
    if (!font)
    {
        fprintf(stderr, "Unable to load font\n");
        return -1;
    }

    fontHeight = font->ascent + font->descent;
    wnwH = WINDOW_TOTAL_LINES_NUM * fontHeight + font->descent;
    wnwW = WINDOW_WIDTH_HEIGHT_RATIO * wnwH;

    rndrClr.red   = 0x0000;
    rndrClr.green = 0x9999;
    rndrClr.blue  = 0x0000;
    rndrClr.alpha = 0x0000;

    XftColorAllocValue(dpy, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen), &rndrClr, &fgClr);

    rndrClr.red   =  0x1100;
    rndrClr.green =  0x0000;
    rndrClr.blue  =  0x1100;
    rndrClr.alpha =  0xffff;
    XftColorAllocValue(dpy, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen), &rndrClr, &bgClr);

    wwDicDefs = window_width_lines(dpy, font, wnwW - margin, dicDefs, &totalLines);

    Atom wmDelete;
    wnw = create_window(dpy, wnwW, wnwH, &wmDelete);
    WP_keep_above(dpy, wnw, _NET_WM_STATE_ADD);

    draw = XftDrawCreate(dpy, wnw, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen));

    maxScrollOffset = (totalLines * fontHeight) - wnwH;

    while (1) {

        XNextEvent(dpy, &ev);

        if(ev.type == Expose)
        {
            XClearWindow(dpy, wnw);

            int y;
            for(int i = 0 ; i < totalLines; ++i)
            {
                y = i * fontHeight - scroll_offset;
                if (y >= 0 && y < (wnwH - 2*fontHeight))
                {
                    XftDrawStringUtf8(draw, &fgClr, font, HORIZ_OFFSET, fontHeight + y, (FcChar8 *)wwDicDefs[i], strlen(wwDicDefs[i]));
                }
                else if(y < (wnwH - fontHeight))
                {
                    char sProgressInfo[64];

                    int strPxW, sLen,
                        midOffset = 0,
                        rectX, rectY, rectW, rectH;

                    unsigned short lineNum = scroll_offset > 0 ? (scroll_offset / fontHeight) : 0;                     sprintf(sProgressInfo, "line %d/%d  %.0f%% (press space to quit)",
                             lineNum + 1, totalLines - 1,
                             100.0f * (float)(i)/(totalLines - 1));
                    sLen = strlen(sProgressInfo);
                    strPxW = get_str_px_width(dpy, font, sProgressInfo);
                    midOffset = (wnwW - strPxW) / 2;

                    // Draw background rectangle

                    rectY = y + fontHeight - font->ascent;
                    rectX = midOffset - HORIZ_OFFSET;
                    rectW = strPxW + 2*HORIZ_OFFSET;
                    rectH = font->ascent + font->descent;

                    XftDrawRect(draw, &fgClr, rectX, rectY, rectW, rectH);

                    XftDrawStringUtf8(draw, &bgClr, font, midOffset, fontHeight + y,
                                      (FcChar8*)sProgressInfo, sLen);
                }

            }

        }

        else if (ev.type == ButtonPress) {
            if (ev.xbutton.button == Button4) { // Scroll up (mouse wheel)
                if (scroll_offset > 0)
                    scroll_offset -= fontHeight;
            }
            else if (ev.xbutton.button == Button5) { // Scroll down
                if ( scroll_offset < maxScrollOffset)
                    scroll_offset += fontHeight;
            }
            // Force redraw
            DW_redraw_window(dpy, wnw);
        }
        else if (ev.type == KeyPress) {
            keySym = XLookupKeysym(&ev.xkey, 0);

            if(keySym == XK_Up)  // Scroll up
            {
                 if (scroll_offset > 0)
                    scroll_offset -= fontHeight;
            }
            else if(keySym == XK_Down)  // Scroll down
            {
                 if ( scroll_offset < maxScrollOffset)
                    scroll_offset += fontHeight;
            }
            else if(keySym == XK_plus || keySym == XK_equal || keySym == XK_KP_Add)
            {
                unsigned short lineNum = scroll_offset > 0 ? (scroll_offset / fontHeight) : 0;
                fontSize = (fontSize + 1) <= 70 ? ++fontSize : fontSize;
                font = resize_font(dpy, screen, font, fontSize);
                fontHeight = font->ascent + font->descent;
                resize_window(dpy, screen, wnw, font, &wnwW, &wnwH);

                free_dictionary_definitons(wwDicDefs);
                wwDicDefs = window_width_lines(dpy, font, wnwW - margin, dicDefs, &totalLines);

                scroll_offset = lineNum * fontHeight;
                maxScrollOffset = (totalLines * fontHeight) - wnwH;
            }
            else if(keySym == XK_minus || keySym == XK_KP_Subtract)
            {
                unsigned short lineNum = scroll_offset > 0 ? (scroll_offset / fontHeight) : 0;
                fontSize = (fontSize - 1) >= 8 ? --fontSize : fontSize;
                font = resize_font(dpy, screen, font, fontSize);
                fontHeight = font->ascent + font->descent;
                resize_window(dpy, screen, wnw, font, &wnwW, &wnwH);

                free_dictionary_definitons(wwDicDefs);
                wwDicDefs = window_width_lines(dpy, font, wnwW - margin, dicDefs, &totalLines);

                scroll_offset = lineNum * fontHeight;
                maxScrollOffset = (totalLines * fontHeight) - wnwH;
            }
            else if(keySym == XK_space)  // Press space key to close the window
                break;

            // Force redraw
            DW_redraw_window(dpy, wnw);
        }
        else if(ev.type == ClientMessage)
        {
            if(ev.xclient.data.l[0] == wmDelete)
                break;
        }
    }

    //*** UTF-8
    XftColorFree(dpy, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen), &fgClr);
    XftColorFree(dpy, DefaultVisual(dpy, screen), DefaultColormap(dpy, screen), &bgClr);
    XftFontClose(dpy, font);
    XftDrawDestroy(draw);

    free_dictionary_definitons(wwDicDefs);
    XDestroyWindow(dpy, wnw);
}

static void free_dictionary_definitons(char **dicDefs)
{
    for(int i = 0 ; dicDefs[i] ; ++i)
      free(dicDefs[i]);

    free(dicDefs);
}

static XftFont *resize_font(Display *dpy, int screen, XftFont *font, unsigned int fontSize)
{
    char fontPattern[64];
    XftFontClose(dpy, font);
    sprintf(fontPattern, "DejaVu Sans:pixelsize=%u", fontSize);

    font = XftFontOpenName(dpy, screen, fontPattern); // You can change the font
    if (!font)
    {
        fprintf(stderr, "Unable to load font\n");
        exit(-1);
    }

    return font;
}

static void resize_window(Display *dpy, int screen, Window wnw, XftFont *font, unsigned int *wnwW, unsigned int *wnwH)
{
    static char fullScreenF = 1;
    int fontHeight = font->ascent + font->descent;

    *wnwH = WINDOW_TOTAL_LINES_NUM * fontHeight + font->ascent;
    *wnwW = WINDOW_WIDTH_HEIGHT_RATIO * (*wnwH);

    int screenW = XDisplayWidth(dpy, screen);
    int screenH = XDisplayHeight(dpy, screen);

    // The window should be without decoration.
    if( ((*wnwH > screenH) || (*wnwW > screenW)) )
    {
        *wnwW = screenW;
        *wnwH = screenH;
        if(fullScreenF)
        {
            WP_full_screen(dpy, wnw, _NET_WM_STATE_ADD);
            XMoveResizeWindow(dpy, wnw, 0, 0, screenW, screenH);
            fullScreenF = 0;
        }
        return;
    }

    if(!fullScreenF)
        WP_full_screen(dpy, wnw, _NET_WM_STATE_REMOVE);

    fullScreenF = 1;
    int x = (screenW - *wnwW) / 2;
    int y = (screenH - *wnwH) / 2;
    XMoveResizeWindow(dpy, wnw, x, y, *wnwW, *wnwH);
}
