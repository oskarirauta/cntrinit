all: world

CC ?= cc
CFLAGS ?= -std=gnu11 -Os -Wall -Wextra
LDFLAGS ?=

# Fully static, position-dependent binary with no runtime dependencies.
# Override STATIC_LDFLAGS if you need e.g. static-pie instead.
STATIC_LDFLAGS ?= -static -no-pie -Wl,-z,relro -Wl,-z,now

INCLUDES += -I./include
DEPFLAGS := -MMD -MP

# Expose the GNU/BSD libc symbols we use (execvpe, strdup, getcwd, signalfd)
# regardless of any -std override (e.g. from the OpenWrt buildroot): a strict
# -std defines __STRICT_ANSI__, which hides them on musl. `override` keeps this
# even when CFLAGS is set on the make command line.
override CFLAGS += -D_GNU_SOURCE

PREFIX ?= /usr/local
DESTDIR ?=

OBJS := \
	objs/util.o \
	objs/config.o \
	objs/usage.o \
	objs/logging.o \
	objs/handler.o \
	objs/reaper.o \
	objs/child.o \
	objs/main.o

world: cntrinit

$(shell mkdir -p objs)

objs/%.o: src/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) $(INCLUDES) -c -o $@ $<

objs/main.o: main.c
	$(CC) $(CFLAGS) $(DEPFLAGS) $(INCLUDES) -c -o $@ $<

cntrinit: $(OBJS)
	$(CC) $(CFLAGS) $(STATIC_LDFLAGS) $^ -o $@ $(LDFLAGS)

-include $(OBJS:.o=.d)

.PHONY: install
install: cntrinit
	install -D -m 0755 cntrinit $(DESTDIR)$(PREFIX)/bin/cntrinit

.PHONY: strip
strip: cntrinit
	strip --strip-all cntrinit

.PHONY: clean
clean:
	@rm -rf objs cntrinit

.PHONY: all world
