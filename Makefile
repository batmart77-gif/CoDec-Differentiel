CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -g -D_POSIX_C_SOURCE=200809L
INCLUDES = -I.
SRC = main.c image.c diff.c bitstream.c
OBJ = $(SRC:.c=.o)
TARGET = codec

IMAGES_DIR = images_test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
	rm -f $(OBJ) $(TARGET)

test:
	./$(TARGET) -i $(IMAGES_DIR)/boat.pgm

.PHONY: all clean test

