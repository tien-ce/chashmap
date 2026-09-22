CC = gcc
CFLAGS = -Wall -Wextra -I./include -g
AR = ar
ARFLAGS = rcs

SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
TEST_DIR = tests

# Static library output name
LIB_NAME = libchashmap.a
# Test executable name
TEST_EXEC = test_runner

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Build both static library and test executable
all: $(LIB_NAME) $(TEST_EXEC)

# Archive rule for static library
$(LIB_NAME): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

# Compile C source into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Compile test runner and link with static library
$(TEST_EXEC): $(TEST_DIR)/main.c $(LIB_NAME)
	$(CC) $(CFLAGS) $< -L. -lchashmap -o $@

clean:
	rm -rf $(OBJ_DIR) $(LIB_NAME) $(TEST_EXEC)

.PHONY: all clean
