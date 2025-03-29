# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g2 -fsanitize=address

TARGET = test_category
SRC = test_category.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
