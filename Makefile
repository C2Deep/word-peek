CC = gcc
SRC_DIR = src
TARGET = main

LIBS = `pkg-config --cflags --libs x11 xft` -lpng -ltesseract -llept  -lcurl -lcjson -lm

SRC = $(wildcard $(SRC_DIR)/*.c)

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $@ $(SRC) $(LIBS)
