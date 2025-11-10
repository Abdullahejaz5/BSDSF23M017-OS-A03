CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS = -lreadline

SRC = src/main.c src/shell.c src/execute.c
OBJ_DIR = obj
BIN_DIR = bin
OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/shell.o $(OBJ_DIR)/execute.o
TARGET = $(BIN_DIR)/myshell

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: all
	./$(TARGET)
