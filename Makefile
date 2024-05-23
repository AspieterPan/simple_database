CC = clang
SRC_DIR = src
INC_DIR = inc
BIN_DIR = bin
DB_DIR = db
CFLAGS = -Wall -Wextra -I$(INC_DIR) -std=c17 -g
EXE = simple_db

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

all: $(EXE)

run: $(EXE)
	./$(EXE) ./db/test.db

.PHONY: test
test:
	python3 ./py/test.py

$(EXE): $(OBJS) 
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: cleandb
cleandb:
	rm -f $(DB_DIR)/*

.PHONY: clean
clean:
	rm -f $(BIN_DIR)/*.o $(EXE) $(DB_DIR)/*
