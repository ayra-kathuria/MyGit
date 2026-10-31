CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

SRC = src/main.c \
      src/init.c \
      src/add.c \
      src/commit.c \
      src/log.c \
      src/status.c \
      src/checkout.c

TARGET = mygit

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
