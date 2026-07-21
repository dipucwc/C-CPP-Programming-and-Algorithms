# Discrete PID controller with anti-windup, C99.
CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDLIBS  := -lm

all: test_pid

test_pid: tests/test_pid.c src/pid.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test: test_pid
	./test_pid

clean:
	rm -f test_pid

.PHONY: all test clean
