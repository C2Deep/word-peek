#include<stdio.h>
#include<string.h>

#include<tesseract/capi.h>
#include<leptonica/allheaders.h>

int image_2_text(const char *image, const char *lang, char *text);

int image_2_text(const char *image, const char *lang, char *text)
{
    // Initialize Tesseract OCR engine
    TessBaseAPI *api = TessBaseAPICreate();
    if (TessBaseAPIInit3(api, NULL, lang)) {
        fprintf(stderr, "Could not initialize tesseract.\n");
        return -1;
    }

    // Set the Page Segmentation Mode (PSM) to PSM_SINGLE_LINE
    TessBaseAPISetPageSegMode(api, PSM_SINGLE_LINE);

    // Load image using Leptonica
    PIX *pixImage = pixRead(image);
    pixSetXRes(pixImage, 150);
    pixSetYRes(pixImage, 150);
    if (!pixImage)
    {
        fprintf(stderr, "Could not read input image.\n");
        TessBaseAPIDelete(api);
        return -1;
    }

    // Set image for OCR
    TessBaseAPISetImage2(api, pixImage);

    // Get OCR result text
    char *Ttext = TessBaseAPIGetUTF8Text(api);
    text[0] = '\0';
    if (Ttext)
    {
        snprintf(text, strlen(Ttext), "%s", Ttext);
        TessDeleteText(Ttext);
    } else
    {
        fprintf(stderr, "OCR failed or no text found.\n");
        return -1;
    }

    // Clean up
    pixDestroy(&pixImage);
    TessBaseAPIEnd(api);
    TessBaseAPIDelete(api);
    return 0;
}

