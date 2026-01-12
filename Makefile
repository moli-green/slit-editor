# Makefile for slit

# --- Variables ---
CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -std=c99 -O2

TARGET = slit

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1

# --- Targets ---

all: $(TARGET)

$(TARGET): slit.c
	$(CC) $(CFLAGS) -o $(TARGET) slit.c

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(MANDIR)
	install -m 644 slit.1 $(DESTDIR)$(MANDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(MANDIR)/slit.1

.PHONY: all clean install uninstall
