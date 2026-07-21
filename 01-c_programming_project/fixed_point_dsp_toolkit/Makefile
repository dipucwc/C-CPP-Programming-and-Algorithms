# Fixed-point DSP toolkit in C99.
# Targets: all (demo + tests), test (build and run all tests), clean.

CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDLIBS  := -lm

SRC     := src/ring_buffer.c src/dsp_filters.c
TESTS   := test_q15 test_ring_buffer test_dsp_filters

all: dsp_demo $(TESTS)

dsp_demo: src/main.c $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test_q15: tests/test_q15.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test_ring_buffer: tests/test_ring_buffer.c src/ring_buffer.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test_dsp_filters: tests/test_dsp_filters.c src/dsp_filters.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test: $(TESTS)
	./test_q15
	./test_ring_buffer
	./test_dsp_filters

clean:
	rm -f dsp_demo $(TESTS)

.PHONY: all test clean
