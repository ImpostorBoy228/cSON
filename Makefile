CC      ?= cc
CFLAGS  ?= -Wall -Wextra -std=c2x -O2
SRC     := src/cSON.c
HDR     := src/cSON.h
BIN     := cSON
SON     := emmm.son
EVAL    := SONEVAL.ass
TEST    := tests/test

all: $(BIN)

$(EVAL): $(SRC) $(HDR) $(SON)
	$(CC) $(CFLAGS) -DCSON_NO_EVALPOINT -o .gen-soneval $(SRC)
	./.gen-soneval --gen-evalpoint $@ $(SON)
	rm -f .gen-soneval

$(BIN): $(SRC) $(HDR) $(EVAL)
	$(CC) $(CFLAGS) -o $@ $(SRC)

$(TEST): tests/test.c $(HDR)
	$(CC) $(CFLAGS) -o $@ tests/test.c

clean:
	rm -f $(BIN) $(TEST) $(EVAL) .gen-soneval

tests: $(TEST)
	./$(TEST)

.PHONY: all clean tests
