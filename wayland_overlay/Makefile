CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic
LDFLAGS ?= -lwayland-client

PROGRAM = overlay
SRCS = overlay.c wlr-layer-shell.c xdg-shell.c ../read_sway.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

clean:
	rm -f $(PROGRAM) $(OBJS)
