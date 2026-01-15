CC = gcc

# Compilation flags : -fPIC obligatoire pour shared lib
CFLAGS = -Wall -Wextra -std=c17 -g -O2 -fPIC -D_POSIX_C_SOURCE=200809L
PFLAGS = -Iinclude

# Link flags : -shared pour produire .so
LFLAGS = -shared

SRC = src/image.c src/diff.c src/normalize.c src/bitstream.c src/vlc.c
OBJ = $(SRC:src/%.c=build/%.o)

LIBDIR = lib
LIBNAME = codec
LIB = $(LIBDIR)/lib$(LIBNAME).so

all: $(LIB)

$(LIB): $(OBJ) | $(LIBDIR)
	$(CC) $(OBJ) $(LFLAGS) -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) $(PFLAGS) -c $< -o $@

build:
	mkdir -p build

$(LIBDIR):
	mkdir -p $(LIBDIR)

clean:
	rm -rf build $(LIBDIR)

.PHONY: all clean
