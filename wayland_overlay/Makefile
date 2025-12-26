CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic
LDFLAGS ?= -lwayland-client

PROGRAM = overlay
SRCS = overlay.c wlr-layer-shell.c xdg-shell.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

clean:
	rm -f $(PROGRAM) $(OBJS)

