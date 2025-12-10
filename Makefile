CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lncurses
TARGET = jogo
SOURCES = main.c game.c ui.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

