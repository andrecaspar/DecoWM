CFLAGS += -std=c99 -Wall -Wextra -g -pedantic -Wold-style-declaration
CFLAGS += -Wmissing-prototypes -Wno-unused-parameter
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
CC     ?= gcc

all: decowm

decowm: decowm.c Makefile
	$(CC) -O3 $(CFLAGS) -o $@ $< -lX11 -lm -lXpm $(LDFLAGS)

install: all
	install -Dm755 decowm $(DESTDIR)$(BINDIR)/decowm

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/decowm

clean:
	rm -f decowm *.o

.PHONY: all install uninstall clean
