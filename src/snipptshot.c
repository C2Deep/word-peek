#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <leptonica/allheaders.h>
#include <png.h>

// Global functions
XImage* SS_get_window_screenshot(Display *dpy, Window wnw);
void SS_put_screenshot_img(Display *dpy, Window wnw, XImage *ssImg);
XImage* SS_grayscale_img(Display *dpy, Window root, XImage *img);
int SS_snippt_shot(Display *display, Window root, int x, int y, int width, int height);
int SS_auto_word_selection(Display *dpy, XImage *img, int startX, int startY, int *wordPos);

// Local functions
static void save_png(const char *filename, XImage *image);
static XImage *crop_image(Display *dpy, Visual *visual, XImage *img, unsigned int *wordHeight);
static void crop_img_dim(int vMax, int *vArray, int vSize, int hMax, int *hArray, int hSize, int *dArray);
static void lept_img_process(char *imgPath, unsigned int wordHeight);
static void lept_calculate_black_white_pix(PIX *pixBin, l_uint32 *blkPixCount, l_uint32 *whtPixCount);
static void lept_calculate_black_white_cols(PIX *pixBin, l_uint32 *blkColCount, l_uint32 *whtColCount);
static l_int32 count_set_bits(l_uint32 n);
static unsigned int max_pixel_contrast_diff_1D(XImage *img, unsigned int yStart, unsigned int yEnd, unsigned int *array, char directionF);

static l_int32 count_set_bits(l_uint32 n) {
    l_int32 count = 0;
    while (n > 0) {
        n &= (n - 1);
        count++;
    }
    return count;
}

static void lept_calculate_black_white_cols(PIX *pixBin, l_uint32 *blkColCount, l_uint32 *whtColCount)
{
    l_uint32 width = pixGetWidth(pixBin),
            height = pixGetHeight(pixBin),
            wpl = pixGetWpl(pixBin),
            *data = pixGetData(pixBin),  *row = NULL,
            colCount[width];

    l_uint8 pixVal;

    *blkColCount = 0;
    *whtColCount = 0;

    for (l_uint32 x = 0, counter = 0; x < width ; ++x, counter = 0)
    {
        for (l_uint32 y = 0, tmp = 0 ; y < height ; ++y)
        {
            row = data + (wpl * y);

            pixVal = GET_DATA_BIT(row, x);
            counter += pixVal;

            if( y && (pixVal != tmp) )
                break;

            tmp = pixVal;
        }
        colCount[x] = counter;

         if(x)
         {
            if(colCount[x] == colCount[x - 1])
                continue;
            *whtColCount += (colCount[x - 1] == 0)        ? 1 : 0;
            *blkColCount += (colCount[x - 1] == height)   ? 1 : 0;
         }

    }
}

static void lept_calculate_black_white_pix(PIX *pixBin, l_uint32 *blkPixCount, l_uint32 *whtPixCount)
{
    l_uint32 width = pixGetWidth(pixBin),
            height = pixGetHeight(pixBin),
            wpl = pixGetWpl(pixBin),
            totalPixCount = width * height,
            *data = pixGetData(pixBin), *row = NULL;

    *blkPixCount = 0;
    *whtPixCount = 0;

    for (l_uint32 y = 0 ; y < height ; ++y)
    {
        row = data + (y * wpl);

        for (l_uint32 x = 0 ; x < wpl ; ++x)
            *blkPixCount += count_set_bits(row[x]);

    }
    *whtPixCount = totalPixCount - *blkPixCount;
}

static void lept_img_process(char *imgPath, unsigned int wordHeight)
{
    PIX *pixOriginal = NULL,
        *pixProcessed = NULL,
        *pixBinary = NULL,
        *pixDeskewed = NULL,
        *pixThreshold = NULL;

    l_float32 angle, conf;

    float scaleFactor = 42.0f/wordHeight,
          scorefract = 0.1f;

    int sx = 32, sy = 32,
        smoothx = 2, smoothy = 2;

    pixOriginal = pixRead(imgPath);
    if (!pixOriginal) {
        fprintf(stderr, "Error: Could not read image from %s\n", imgPath);
        exit(-1);
    }

    if (pixGetDepth(pixOriginal) > 8)
        pixProcessed = pixConvertRGBToLuminance(pixOriginal);
    else
        pixProcessed = pixClone(pixOriginal);

    if (!pixProcessed)
    {
        fprintf(stderr, "Error converting to grayscale.\n");
        pixDestroy(&pixOriginal);
        exit(-1);
    }

    if (pixProcessed->w < 1000 || pixProcessed->h < 1000)
    {
        PIX *pixScaled = pixScale(pixProcessed, scaleFactor, scaleFactor);
        pixDestroy(&pixProcessed);
        pixProcessed = pixScaled;
        if (!pixProcessed)
        {
            fprintf(stderr, "Error scaling image.\n");
            pixDestroy(&pixOriginal);
            exit(-1);
        }
    }

    pixDeskewed = pixFindSkewAndDeskew(pixProcessed, 0, &angle, &conf);
    if (pixDeskewed)
    {
        pixDestroy(&pixProcessed);
        pixProcessed = pixDeskewed;
    }

    if ((pixOtsuAdaptiveThreshold(pixProcessed, sx, sy, smoothx, smoothy, scorefract, &pixThreshold, &pixBinary)))
    {
        fprintf(stderr, "Error binarizing image.\n");
        pixDestroy(&pixOriginal);
        pixDestroy(&pixProcessed);
        pixDestroy(&pixThreshold);
        exit(-1);
    }
    pixDestroy(&pixThreshold);


    l_uint32 blkColCount, whtColCount;
    lept_calculate_black_white_cols(pixBinary, &blkColCount, &whtColCount);

    if(blkColCount > whtColCount)
        pixInvert(pixBinary, pixBinary);      // Invert image binary colors

    else if(blkColCount == whtColCount)
    {
        l_uint32 blkPixCount, whtPixCount;
        lept_calculate_black_white_pix(pixBinary, &blkPixCount, &whtPixCount);

        if(blkPixCount > whtPixCount)
            pixInvert(pixBinary, pixBinary);      // Invert black and white colors

    }

    if(pixWrite("processedWord.png", pixBinary, IFF_PNG))
    {
        fprintf(stderr, "Error wrtiting processed image data to processedWord.png file\n");
        exit(-1);
    }

    pixDestroy(&pixOriginal);
    pixDestroy(&pixProcessed);
    pixDestroy(&pixBinary);
}

int SS_snippt_shot(Display *display, Window root, int x, int y, int width, int height)
{
    XWindowAttributes attrs;
    XImage *image = XGetImage(display, root, x, y, width, height, AllPlanes, ZPixmap);
    if (!image) {
        fprintf(stderr, "Failed to get image from root window.\n");
        XCloseDisplay(display);
        return 1;
    }

    if (!XGetWindowAttributes(display, root, &attrs))
    {
        fprintf(stderr, "Unable to get window attributes\n");
        exit(-1);
    }

      unsigned int wordHeight = 0;
      XImage *cropImg = crop_image(display, attrs.visual, image, &wordHeight);

      if(cropImg)
      {
            save_png("cropedImg.png", cropImg);
            lept_img_process("cropedImg.png", wordHeight);
            XDestroyImage(cropImg);
            remove("cropedImg.png");
      }

    XDestroyImage(image);
}

// Save png as grayscale
static void save_png(const char *filename, XImage *img)
{
    unsigned long pixel;

    char redOffset, greenOffset, blueOffset;        // red, green, blue byte offsets from 4 bytes pixels

    unsigned int imgW = img->width,
                 imgH = img->height,

                 bpp = img->bits_per_pixel >> 3,  // bytes per pixel
                 bpl = img->bytes_per_line,       // bytes per line
                 offset;

    png_bytep *pRows = (png_bytep *)malloc(sizeof(png_bytep) * imgH);
    if(!pRows)
    {
        fprintf(stderr, "Couldn't allocate memory for png rows.\n");
        exit(-1);
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        perror("fopen");
        return;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
    {
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info)
    {
        png_destroy_write_struct(&png, NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png)))
    {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return;
    }

    png_init_io(png, fp);

    png_set_IHDR(png, info, imgW, imgH, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png, info);

    png_bytep data = (png_bytep)img->data;
    unsigned long counter = 0;

    png_byte r, g, b;

    redOffset = (img->red_mask >> 8)  == 0 ? 0 :
                (img->red_mask >> 16) == 0 ? 1 :
                (img->red_mask >> 24) == 0 ? 2 : 3;

     greenOffset = (img->green_mask >> 8)  == 0 ? 0 :
                   (img->green_mask >> 16) == 0 ? 1 :
                   (img->green_mask >> 24) == 0 ? 2 : 3;

    blueOffset = (img->blue_mask >> 8)  == 0 ? 0 :
                 (img->blue_mask >> 16) == 0 ? 1 :
                 (img->blue_mask >> 24) == 0 ? 2 : 3;

    for (int y = 0; y < imgH; y++)
    {
        pRows[y] = (png_bytep)malloc(imgW * 3 * sizeof(png_byte));
        for (int x = 0; x < imgW; x++)
        {
            offset = (y * bpl) + (x * bpp);
            r = data[offset + redOffset];
            g = data[offset + greenOffset];
            b = data[offset + blueOffset];
            pRows[y][(x * 3) + 0] = r;
            pRows[y][(x * 3) + 1] = g;
            pRows[y][(x * 3) + 2] = b;
        }

    }

    png_write_image(png, pRows);
    png_write_end(png, NULL);

    for (int y = 0; y < imgH ; y++)
        free(pRows[y]);

    free(pRows);

    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

XImage* SS_get_window_screenshot(Display *dpy, Window wnw)
{
    XWindowAttributes attrs;
    if (!XGetWindowAttributes(dpy, wnw, &attrs)) {
        fprintf(stderr, "Unable to get window attributes\n");
        exit(-1);
    }

    XImage *ssImg = XGetImage(dpy, wnw, 0, 0, attrs.width, attrs.height, AllPlanes, ZPixmap);
    if (!ssImg) {
        fprintf(stderr, "Unable to make screenshot image for the window.\n");
        XCloseDisplay(dpy);
        exit(-1);
    }

    return ssImg;
}

void SS_put_screenshot_img(Display *dpy, Window wnw, XImage *ssImg)
{
    int imgW = ssImg->width,
        imgH = ssImg->height,
        dataSize = 0;

    XImage *chkImg;

    char *chkImgData,
         *ssImgData = ssImg->data;

    char evCountF = 0;
    char wnwMapF = 0;   // window map flag

    dataSize = imgW * imgH;

    // Wait for the window to be mapped
    XEvent ev;
    for (;;)
    {
        XNextEvent(dpy, &ev);

        if (ev.xany.window == wnw)
            if (ev.type == MapNotify)
                break;
    }

    // Wait for final Expose event
    for (;;)
    {
        XNextEvent(dpy, &ev);
        if(ev.type == Expose)
        {
            if(ev.xexpose.count > 0)
                evCountF = 1;

            if (ev.xexpose.count == 0 && evCountF)
                break;
        }

    }

   GC gc = XCreateGC(dpy, wnw, 0, NULL);

    for( ;; )
    {
        XPutImage(dpy, wnw, gc, ssImg, 0, 0, 0, 0, imgW, imgH);
        XFlush(dpy);
        chkImg = SS_get_window_screenshot(dpy, wnw);
        chkImgData = chkImg->data;

        if(!(memcmp(ssImgData, chkImgData, dataSize)))
        {
            XDestroyImage(chkImg);
            break;
        }

        XDestroyImage(chkImg);
     }

      XFreeGC(dpy, gc);
}

XImage* SS_grayscale_img(Display *dpy, Window root, XImage *img)
{
    XImage *gsImg = SS_get_window_screenshot(dpy, root);

    char *imgData = img->data,
         *gsData = gsImg->data;

    char redOffset, greenOffset, blueOffset;        // red, green, blue byte offsets from 4 bytes pixels

    unsigned int imgW = img->width,
                 imgH = img->height,

                 bpp = img->bits_per_pixel >> 3,  // bytes per pixel
                 bpl = img->bytes_per_line,       // bytes per line
                 offset;

    unsigned char r, g, b, gsByte;               // grayscaled byte

    redOffset = (img->red_mask >> 8)  == 0 ? 0 :
                (img->red_mask >> 16) == 0 ? 1 :
                (img->red_mask >> 24) == 0 ? 2 : 3;

     greenOffset = (img->green_mask >> 8)  == 0 ? 0 :
                   (img->green_mask >> 16) == 0 ? 1 :
                   (img->green_mask >> 24) == 0 ? 2 : 3;

    blueOffset = (img->blue_mask >> 8)  == 0 ? 0 :
                 (img->blue_mask >> 16) == 0 ? 1 :
                 (img->blue_mask >> 24) == 0 ? 2 : 3;

    for (int y = 0 ; y < imgH ; ++y)
    {
        for (int x = 0 ; x < imgW ; ++x)
        {

            offset = (y * bpl) + (x * bpp);
            r = imgData[offset + redOffset];
            g = imgData[offset + greenOffset];
            b = imgData[offset + blueOffset];

           gsByte = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);  // grayscale

           gsData[offset + redOffset] = gsByte;
           gsData[offset + greenOffset] = gsByte;
           gsData[offset + blueOffset] = gsByte;
        }
    }

    return gsImg;
}

static void crop_img_dim(int vMax, int *vArray, int vSize, int hMax, int *hArray, int hSize, int *dArray)
{
    int hThreshold = 0, vThreshold = 0,
        vStart = 0, vEnd = 0,
        hStart = 0, hEnd = 0,
        x,  y;

    char xStopF = 0, yStopF = 0;

    hThreshold = hMax * 0.40f;
    vThreshold = vMax * 0.40f;

    // left-right and top-buttom
    for(x = y = 0 ; y < vSize || x < hSize ; ++x, ++y)
    {
        if(hArray[x] >= hThreshold && !xStopF) { hStart = (x - 3 < 0) ? 0 : x - 3;  xStopF = 1;}
        if(vArray[y] >= vThreshold && !yStopF) { vStart = (y - 3 < 0) ? 0 : y - 3;  yStopF = 1;}

        if(xStopF && yStopF)
            break;

    }

    xStopF = yStopF = 0;

    // right-left and buttom-top
    for(x = hSize - 1, y = vSize - 1 ; x > 0 || y > 0 ; --x, --y)
    {
        if(hArray[x] >= hThreshold && !xStopF) { hEnd = (x + 3 >= hSize) ? hSize - 1 : x + 3; xStopF = 1;}
        if(vArray[y] >= vThreshold && !yStopF) { vEnd = (y + 3 >= vSize) ? vSize - 1 : y + 3; yStopF = 1;}
        if(xStopF && yStopF)
            break;
    }

    dArray[0] = hStart;
    dArray[1] = hEnd;
    dArray[2] = vStart;
    dArray[3] = vEnd;
}

static XImage *crop_image(Display *dpy, Visual *visual, XImage *img, unsigned int *wordHeight)
{
    XImage *cropImg = NULL;
    unsigned int cropImgW, cropImgH,  // crop image width and height
                 imgW = img->width,
                 imgH = img->height,

                 bpp = img->bits_per_pixel >> 3,  // bytes per pixel
                 bpl = img->bytes_per_line,       // bytes per line
                 offset;

    int yStart = 0, yEnd = imgH,
        vStart = 0, vEnd = imgH,
        hStart = 0, hEnd = 0,
        vMax = 0,   hMax = 0,
        *vArray = NULL, *hArray = NULL,
        dArray[4] = {0};  // Dimension array

    unsigned char greenOffset;

    char *cropData,
         *imgData = img->data;

     greenOffset = (img->green_mask >> 8)  == 0 ? 0 :
                   (img->green_mask >> 16) == 0 ? 1 :
                   (img->green_mask >> 24) == 0 ? 2 : 3;


    if(!(vArray = (int *)calloc(imgH, sizeof(int))))
    {
        fprintf(stderr, "couldn't allocate memeory.\n");
        exit(-1);
    }

    if(!(hArray = (int *)calloc(imgW, sizeof(int))))
    {
        fprintf(stderr, "couldn't allocate memeory.\n");
        exit(-1);
    }

    hMax = max_pixel_contrast_diff_1D(img, yStart, yEnd, hArray, 0);
    vMax = max_pixel_contrast_diff_1D(img, yStart, yEnd, vArray, 1);

    crop_img_dim(vMax, vArray, imgH, hMax, hArray, imgW, dArray);

    hStart = dArray[0];
    hEnd   = dArray[1];
    vStart = dArray[2];
    vEnd   = dArray[3];

    // Crop image width
    cropImgW = hEnd - hStart + 1;
    // Crop image height
    cropImgH = vEnd - vStart + 1;

    *wordHeight = cropImgH;

    if(cropImgH && cropImgW)
    {
        if( !(cropData = (char *)calloc(cropImgW * cropImgH * bpp, 1)) )
        {
            fprintf(stderr, "Couldn't allocate memory.\n");
            exit(-1);
        }

        cropImg = XSubImage(img, hStart, vStart, cropImgW, cropImgH);
    }

    free(vArray);
    free(hArray);
    return cropImg;
}

int SS_auto_word_selection(Display *dpy, XImage *img, int startX, int startY, int *wordPos)
{
    int screenW = XDisplayWidth(dpy, XDefaultScreen(dpy)), screenH = XDisplayHeight(dpy, XDefaultScreen(dpy)),
                 subImgW = 20,  subImgH = 10,
                 relStartX = 0, relStartY = 0,  // Relative startX and startY in subImg
                 subImgX = 0, subImgY = 0,
                 yStart = 0, yEnd = subImgH,
                 vStart = 0, vEnd = subImgH,
                 hStart = 0, hEnd = 0,
                 hMax = 0, vMax = 0,
                 hThreshold = 0, vThreshold = 0,
                 bpp = img->bits_per_pixel >> 3,  // bytes per pixel
                 bpl = 0,   // bytes per line
                 pbcMax = 0, pbcMin = 0xffff, // Arbitrarily large value
                 pbcAvg = 0,
                 *hArray = NULL, *vArray = NULL;

    unsigned char vStopUpF = 0, vStopDnF = 0,
                  hStopRgtF = 0, hStopLftF = 0,
                  charF = 0,
                  charHeightF = 0, charHeight = 0,
                  pbcLF = 0, pbcRF = 0,     // left and right pixels between character flags
                  subImgNames[100] = {0};

    XImage *autoSelectImg = NULL,
           *subImg = NULL;

    subImgX = startX  - (subImgW/2);
    subImgY = startY - (subImgH/2);
    subImg = XSubImage(img, subImgX, subImgY, subImgW, subImgH);

    char pixMax[100];

    pixMax[0] = 5;
    for(int i = 1 ; i < 100 ; ++i)
        pixMax[i] = 2.7 * (i + 1);

    while(!hStopLftF || !hStopRgtF || !vStopUpF || !vStopDnF)
    {
        hStopLftF = hStopRgtF = vStopUpF = vStopDnF = 0;

        if(!(vArray = (unsigned int *)calloc(subImgH, sizeof(unsigned int))))
        {
            fprintf(stderr, "couldn't allocate memeory.\n");
            exit(-1);
        }

        vMax = max_pixel_contrast_diff_1D(subImg, yStart, yEnd, vArray, 1);

        vThreshold = vMax * 0.40f;

        relStartY = startY - subImgY;
        for(int y = relStartY ; y >= 0 ; --y)
        {
            yStart = y + 1;
            if(vArray[y] < vThreshold)
            {
                vStart = y;
                vStopUpF = 1;
                break;
            }

        }

        for(int y = relStartY ; y < subImgH ; ++y)
        {
            yEnd =  (y + 1) < subImgH ? y + 1 : y;
            if(vArray[y] < vThreshold)
            {
                vEnd = (y + 1) < subImgH ? y + 1 : y;
                vStopDnF = 1;
                break;
            }
        }
        free(vArray);

        if(charHeight && ( (vEnd - vStart) > (charHeight * 3) ))
            return 0;

        if(!(hArray = (unsigned int *)calloc(subImgW, sizeof(unsigned int))))
        {
            fprintf(stderr, "couldn't allocate memeory.\n");
            exit(-1);
        }

        hMax = max_pixel_contrast_diff_1D(subImg, yStart, yEnd, hArray, 0);

        // max pixel contrast more than 25
        if(hMax < 25)
        {
            subImgW += 8;
            subImgX -= 4;

            if( ( (subImgX + subImgW) > screenW) || (subImgX < 0) )
                return 0;

            subImg = XSubImage(img, subImgX, subImgY, subImgW, subImgH);

            free(hArray);
            continue;
        }

        if(charHeightF)
        {
            hThreshold = hMax * 0.20f;

            relStartX = startX - subImgX;

            for(int x = relStartX, pixCount = 0 ; x < subImgW ; ++x)
            {
                if(hArray[x] >= hThreshold)
                {
                    if(charF && pixCount)
                    {
                        pbcRF = 1;
                        pbcMax = (pixCount > pbcMax) ? pixCount : pbcMax;
                        pbcMin = (pbcMin > pixCount) ? pixCount : pbcMin;
                        pbcAvg = (pbcMax + pbcMin) / 2.0;
                    }

                    charF = 1;
                    pixCount = 0;
                }
                if(hArray[x] < hThreshold && charF)
                {
                    ++pixCount;
                    if(pbcLF && pbcRF)
                    {
                        if(pixCount == pixMax[pbcAvg - 1])
                        {

                            hEnd = x - pixMax[pbcAvg - 1] + 2;
                            hStopRgtF = 1;
                            break;
                        }
                    }
                }
            }
            charF = 0;

            for(int x = relStartX, pixCount = 0 ; x >= 0; --x)
            {
                if(hArray[x] >= hThreshold)
                {
                    if(charF && pixCount)
                    {
                        pbcLF = 1;
                        pbcMax = (pixCount > pbcMax) ? pixCount : pbcMax;
                        pbcMin = (pbcMin > pixCount) ? pixCount : pbcMin;
                        pbcAvg = (pbcMax + pbcMin) / 2.0;
                    }
                    charF = 1;
                    pixCount = 0;
                }

                if(hArray[x] < hThreshold && charF)
                {
                    ++pixCount;

                    if(pbcLF && pbcRF)
                    {
                        if(pixCount == pixMax[pbcAvg - 1])
                        {
                            hStart = x + pixMax[pbcAvg - 1] - 1;
                            hStopLftF = 1;
                            break;
                        }
                    }
                }
            }
            charF = 0;

            free(hArray);

            if(!hStopLftF || !hStopRgtF)
            {
                XDestroyImage(subImg);
                if(!hStopLftF && !hStopRgtF)
                {
                    subImgW += 8;
                    subImgX -= 4;
                }
                else if(!hStopLftF)
                {
                    subImgW += 4;
                    subImgX -= 4;
                }
                else // if(!hStopRgtF)
                    subImgW += 4;

                if( ( (subImgX + subImgW) > screenW) || (subImgX < 0) )
                    return 0;


                subImg = XSubImage(img, subImgX, subImgY, subImgW, subImgH);
            }
        }

        if(!vStopUpF || !vStopDnF)
        {
            XDestroyImage(subImg);
            if(!vStopUpF && !vStopDnF)
            {
                subImgH += 8;
                subImgY -= 4;
            }
            else if(!vStopUpF)
            {
                subImgH += 4;
                subImgY -= 4;
            }
            else // if(!vStopDnF)
                subImgH += 4;

            if( ( (subImgY + subImgH) > screenH ) || (subImgY < 0) )
                return 0;

            subImg = XSubImage(img, subImgX, subImgY, subImgW, subImgH);

            charHeightF = 0;

        }
        else
        {
            charHeightF = 1;

            if(!charHeight)
                charHeight = vEnd - vStart;
        }
    }

    if((vEnd - vStart) > 1)
    {
        wordPos[0] = subImgX + hStart - 2;
        wordPos[1] = subImgY + vStart - 2;
        wordPos[2] = hEnd - hStart + 4;   // word width;
        wordPos[3] = vEnd - vStart + 4;   // word height

        autoSelectImg = XSubImage(subImg, hStart, vStart, hEnd - hStart, vEnd - vStart);
        save_png("autoWordSelection.png", autoSelectImg);
        lept_img_process("autoWordSelection.png", wordPos[3]);
        XDestroyImage(autoSelectImg);
        remove("autoWordSelection.png");
        return 1;
    }

}

static unsigned int max_pixel_contrast_diff_1D(XImage *img, unsigned int yStart, unsigned int yEnd, unsigned int *array, char directionF)
{
    unsigned int imgW = img->width,
                 imgH = img->height,
                 bpp = img->bits_per_pixel >> 3,  // bytes per pixel
                 bpl = img->bytes_per_line,       // bytes per line
                 offset,
                 max = 0;

    unsigned char *imgData = img->data,
                  greenOffset,
                  imgRow[imgW],
                  imgCol[imgH];

                   greenOffset = (img->green_mask >> 8)  == 0 ? 0 :
                   (img->green_mask >> 16) == 0 ? 1 :
                   (img->green_mask >> 24) == 0 ? 2 : 3;

    if(directionF)
    {
        for (int y = 0 ; y < imgH ; ++y)
        {
            for (int x = 0 ; x < imgW ; ++x)
            {
                offset = (y * bpl) + (x * bpp);
                imgRow[x] = imgData[offset + greenOffset];

                    if(x)
                        array[y] = abs(imgRow[x] - imgRow[x - 1]) > array[y] ? abs(imgRow[x] - imgRow[x - 1]) : array[y];
            }
            max = array[y] > max ? array[y] : max;
        }
    }
    else
    {
        for(int x = 0 ; x < imgW ; ++x)
        {
            for(int y = 0 ; y < yEnd ; ++y)
            {
                offset = (y * bpl) + (x * bpp);
                imgCol[y] = imgData[offset + greenOffset];

                if (y && (y >= yStart))
                    array[x] = abs(imgCol[y] - imgCol[y - 1]) > array[x] ?  abs(imgCol[y] - imgCol[y - 1]) : array[x];
            }
            max = array[x] > max ? array[x] : max;
        }
    }
    return max;
}
