CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
SRC = src/main.c src/shell.c src/execute.c src/vars.c
OBJ = $(SRC:src/%.c=obj/%.o)
BIN = bin/myshell

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN)

obj/%.o: src/%.c | obj
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

obj:
	mkdir -p obj

clean:
	rm -rf obj bin
