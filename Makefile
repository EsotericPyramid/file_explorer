CC=clang++


CFLAGS=-std=c++17 -g -Wall -Wextra -lncurses
SRC=file_explorer.cpp
TARGET=file_explorer
all: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean: 
	rm -f $(TARGET)