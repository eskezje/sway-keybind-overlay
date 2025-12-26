CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic

PROGRAMS = sway_keys

.PHONY: all clean

all: $(PROGRAMS)

sway_keys: main.o read_sway.o
	$(CC) -o $@ $^

main.o: main.c read_sway.h
	$(CC) -c $< $(CFLAGS)

read_sway .o: read_sway.c read_sway.h
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f *.o $(PROGRAMS)
