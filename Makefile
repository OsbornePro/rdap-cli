CC ?= cc
CFLAGS ?= -O2
CPPFLAGS ?=
WARNINGS = -Wall -Wextra -Wpedantic
PKG_CONFIG ?= pkg-config

CPPFLAGS += $(shell $(PKG_CONFIG) --cflags libcurl json-c)
LDLIBS += $(shell $(PKG_CONFIG) --libs libcurl json-c)

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

TARGET = rdap
SRC = src/rdap.c

.PHONY: all clean install uninstall test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -std=c11 $(SRC) -o $@ $(LDLIBS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 0755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"

uninstall:
	rm -f "$(DESTDIR)$(BINDIR)/$(TARGET)"

test: $(TARGET)
	sh tests/smoke.sh
