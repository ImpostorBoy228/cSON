CC      ?= cc
CFLAGS  ?= -Wall -Wextra -std=c23 -O2
SRC     := src/cSON.c
BIN     := cSON

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BIN)

.PHONY: clean
