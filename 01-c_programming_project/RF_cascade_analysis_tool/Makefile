# RF cascade analysis tool, C99.
# Targets: all (demo + test), test (build and run tests), clean.

CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDLIBS  := -lm

all: rf_budget test_rf_cascade

rf_budget: src/main.c src/rf_cascade.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test_rf_cascade: tests/test_rf_cascade.c src/rf_cascade.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test: test_rf_cascade
	./test_rf_cascade

clean:
	rm -f rf_budget test_rf_cascade

.PHONY: all test clean
