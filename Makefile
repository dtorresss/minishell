CC = gcc
CFLAGS = -Wall -Wextra -Werror
LIBS = -L. -lparser
SRC = myshell.c
OUT = myshell

$(OUT): $(SRC) libparser.a
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS) -static

all: $(OUT)

clean:
	rm -f $(OUT)
