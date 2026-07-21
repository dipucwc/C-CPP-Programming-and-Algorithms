# COBS + CRC-16 packet protocol, C99.
CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude

all: test_packet

test_packet: tests/test_packet.c src/packet_protocol.c
	$(CC) $(CFLAGS) $^ -o $@

test: test_packet
	./test_packet

clean:
	rm -f test_packet

.PHONY: all test clean
