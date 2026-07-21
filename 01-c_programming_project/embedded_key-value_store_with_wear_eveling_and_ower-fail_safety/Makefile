# Wear-leveling key-value store on emulated NOR flash, C99.
CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude

all: test_kv

test_kv: tests/test_kv.c src/flash_sim.c src/kv_store.c
	$(CC) $(CFLAGS) $^ -o $@

test: test_kv
	./test_kv

clean:
	rm -f test_kv

.PHONY: all test clean
