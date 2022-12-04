CC=g++
LIBS=sdl2
SRC=main.cpp

all: main

main: main.cpp
	$(CC) -o main $(SRC) `pkgconf --libs $(LIBS)` -ggdb
