# Simple Makefile to build the notepad demo
CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall
SRC = src/main.cpp
BIN_DIR = bin
TARGET = $(BIN_DIR)/notepad
CC=g++
CFLAGS=-std=c++17 -I.. -I../../_libraries/backages/GRAPHICS -I../../_libraries/backages/IO/dataformats/json/headers
SRC=src/main.cpp src/text_box_editor.cpp
BIN=bin/notepad

all: $(BIN)

build:
	mkdir -p build bin

build/main.o: src/main.cpp | build
	$(CC) $(CFLAGS) -c src/main.cpp -o build/main.o


build/text_box_editor.o: src/text_box_editor.cpp | build
	$(CC) $(CFLAGS) -c src/text_box_editor.cpp -o build/text_box_editor.o

build/gui_gtk_like.o: src/gui_gtk_like.cpp | build
	$(CC) $(CFLAGS) -c src/gui_gtk_like.cpp -o build/gui_gtk_like.o

# Compile GRAPHICS implementation sources needed by the product
build/graphics_windows.o: ../../_libraries/backages/GRAPHICS/windows/source/windows.cpp | build
	$(CC) $(CFLAGS) -c ../../_libraries/backages/GRAPHICS/windows/source/windows.cpp -o build/graphics_windows.o

build/graphics_components.o: ../../_libraries/backages/GRAPHICS/components/source/components.cpp | build
	$(CC) $(CFLAGS) -c ../../_libraries/backages/GRAPHICS/components/source/components.cpp -o build/graphics_components.o

# dataformats JSON implementation (required by windows.cpp)
build/dataformats_json.o: ../../_libraries/backages/IO/dataformats/json/source/json.cpp | build
	$(CC) $(CFLAGS) -c ../../_libraries/backages/IO/dataformats/json/source/json.cpp -o build/dataformats_json.o

# GRAPHICS charts implementation (Graph/Canvas used by windows.cpp)
build/graphics_charts.o: ../../_libraries/backages/GRAPHICS/charts/source/graphics.cpp | build
	$(CC) $(CFLAGS) -c ../../_libraries/backages/GRAPHICS/charts/source/graphics.cpp -o build/graphics_charts.o

$(BIN): build/main.o build/text_box_editor.o build/gui_gtk_like.o build/graphics_windows.o build/graphics_components.o build/dataformats_json.o build/graphics_charts.o
	$(CC) $(CFLAGS) build/main.o build/text_box_editor.o build/gui_gtk_like.o build/graphics_windows.o build/graphics_components.o build/dataformats_json.o build/graphics_charts.o -o $(BIN)

clean:
	rm -rf build bin

.PHONY: all clean
