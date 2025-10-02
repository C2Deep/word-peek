# word-peek
word-peek lets you click on any word in your screen and instantly fetch its dictionary definition, synonyms, antonyms, and example sentences. A lightweight, fast, and convenient tool for learners, readers and writers.

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
   just click computer mouse right button in the **center** of the word. If the word consist of 2 words  (ex. back up) click in the space between the two words.
