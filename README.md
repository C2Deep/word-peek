# word-peek
word-peek lets you click/select on any word in your screen and instantly fetch its dictionary definition, synonyms, antonyms, and example sentences. A lightweight, fast, and convenient tool for learners, readers and writers.

## Build Requirements
Compiler toolchain (gcc, make, etc. — install via build-essential on Debian/Ubuntu).

## Dependencies
Debian/Ubuntu:
```
sudo apt-get update
sudo apt-get install -y \
  libx11-dev \
  libxfixes-dev \
  libxft-dev \
  libpng-dev \
  tesseract-ocr \
  libtesseract-dev \
  libleptonica-dev \
  libcurl4-openssl-dev \
  libcjson-dev
```

## Build
```
make
```

## Usage
./main

## How to use 
After you run the program there are two ways to select a word from your computer screen:
- manual selection:
    simply keep pressing the left mouse button and select the word and make sure the whole word is inside the selection box.
  
- automatic selection:
   just click once computer mouse right button in the **center** of the word. If the word consist of 2 words  (ex. back up) click in the space between the two words.

## Technologies Used
- X Window System Libraries.
- [Tesseract OCR (Optical Character Recognition)](https://github.com/tesseract-ocr): Tesseract OCR is a software that can recognize text in images and convert it to various formats.
- [Leptonica](http://www.leptonica.org/): The Leptonica image processing and analysis source code comes with a very weakly restricted copyright license. 
- [libcurl](https://curl.se/libcurl/): libcurl is a free and easy-to-use client-side URL transfer library, supporting many protocols and features. 
- [ibpbg](http://libpng.org/pub/png/libpng.html): Library for reading and writing PNG image files.
- [cJSON](https://github.com/DaveGamble/cJSON): Ultralightweight JSON parser in ANSI C.
  
## Tip
Make a custom shortcut for this program so that you can launch it while you reading, browsing, watching video, ...etc.
