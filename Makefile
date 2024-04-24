CC = clang
CFLAGS = -Wall -Wextra -Iinclude -std=c17
SRC_DIR = src
INC_DIR = include
BIN_DIR = bin

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

all: my_program

run: my_program
	./my_program

my_program: $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(CFLAGS) -c -o $@ $<

clean: 
	rm -f $(BIN_DIR)/*.o my_program
