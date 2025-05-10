CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -D_XOPEN_SOURCE=700

BUILD_DIR = $(HOME)/build-acnn
OBJ_DIR = $(BUILD_DIR)/obj

SRC_DIR = src
SOURCES = $(SRC_DIR)/acnn-block.c $(SRC_DIR)/acnn-inode.c $(SRC_DIR)/acnn-utils.c $(SRC_DIR)/acnn-dir.c $(SRC_DIR)/acnn-file.c $(SRC_DIR)/acnn-main.c
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
TARGET = $(BUILD_DIR)/run-acnn

all: $(BUILD_DIR) $(OBJ_DIR) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)