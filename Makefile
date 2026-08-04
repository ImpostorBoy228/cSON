CC      ?= cc
CFLAGS  ?= -Wall -Wextra -std=c2x -O2
SRC     := src/cSON.c
HDR     := src/cSON.h
COMPILER := src/SonCompiler.c
BIN     := cSON
SON     := emmm.son
EVAL    := SONEVAL.ass
TEST    := tests/test

all: $(BIN) SonCompiler

$(EVAL): $(COMPILER) $(HDR) $(SON)
	$(CC) $(CFLAGS) -o .gen-soneval $(COMPILER)
	./.gen-soneval $(SON) $@
	rm -f .gen-soneval

$(BIN): $(SRC) $(HDR) $(EVAL)
	$(CC) $(CFLAGS) -o $@ $(SRC)

SonCompiler: $(COMPILER) $(HDR)
	$(CC) $(CFLAGS) -o $@ $(COMPILER)

$(TEST): tests/test.c $(HDR)
	$(CC) $(CFLAGS) -o $@ tests/test.c

clean:
	rm -f $(BIN) $(TEST) $(EVAL) .gen-soneval

tests: $(TEST)
	./$(TEST)

.PHONY: all clean tests SonCompiler
