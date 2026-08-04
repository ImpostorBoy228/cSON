CC      ?= cc
CFLAGS  ?= -Wall -Wextra -std=c2x -O2
SRC     := src/cSON.c
BIN     := cSON
TEST    := tests/test

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST): tests/test.c src/cSON.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BIN) $(TEST)

tests: $(TEST)
	./$(TEST)

.PHONY: clean tests
