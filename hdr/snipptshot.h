#ifndef SINPPTSHOT_H_
#define SNIPPTSHOT_H_

int SS_snippt_shot(Display *display, Window root,  int x, int y, int width, int height);
XImage* SS_get_window_screenshot(Display *dpy, Window wnw);
void SS_put_screenshot_img(Display *dpy, Window wnw, XImage *ssImg);
XImage* SS_grayscale_img(Display *dpy, Window root, XImage *img);
int SS_auto_word_selection(Display *dpy, XImage *img, int startX, int startY, int *wordPos);
#endif
