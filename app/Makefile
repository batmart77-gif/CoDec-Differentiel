CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -g -O2 -D_POSIX_C_SOURCE=200809L

# Chemins vers la lib
CODEC_DIR = ../codec
CODEC_INC = $(CODEC_DIR)/include
CODEC_LIB = $(CODEC_DIR)/lib

# Inclure <codec.h>
PFLAGS = -Iinclude -I$(CODEC_INC)

# Link : -lcodec + rpath vers ../codec/lib
# (important : objets AVANT les -l...)
LFLAGS = -Wl,-rpath,$(CODEC_LIB) -L$(CODEC_LIB) -lcodec

SRC = src/main.c
OBJ = $(SRC:src/%.c=build/%.o)

TARGET = codec_app

all: $(CODEC_LIB)/libcodec.so $(TARGET)

$(CODEC_LIB)/libcodec.so:
	$(MAKE) -C $(CODEC_DIR)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LFLAGS) -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) $(PFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build $(TARGET)

clean-all: clean
	$(MAKE) -C $(CODEC_DIR) clean

.PHONY: all clean clean-all
