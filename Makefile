CC=g++
LIBS=sdl2
SRC=main.cpp
STD=c++17

all: main

main: main.cpp
	$(CC) -o main $(SRC) `pkgconf --libs $(LIBS)` -ggdb -std=$(STD)
