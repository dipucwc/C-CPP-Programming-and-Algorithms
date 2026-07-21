# Cooperative tick-driven task scheduler, C99.
CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude

all: test_scheduler

test_scheduler: tests/test_scheduler.c src/scheduler.c
	$(CC) $(CFLAGS) $^ -o $@

test: test_scheduler
	./test_scheduler

clean:
	rm -f test_scheduler

.PHONY: all test clean
