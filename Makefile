# Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g2 #-fsanitize=address

TARGET = test_category
SRC = test_category.c commands.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ) 
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c commands.h category.h
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)
