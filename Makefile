all: world

CC?=gcc
CFLAGS?=-Wall -fPIC -O2
STATIC_LDFLAGS?=-Wl,-z -Wl,now -Wl,-z -Wl,relro -fuse-ld=bfd -znow -zrelro -static
INCLUDES+= -I./include

OBJS:= \
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

objs/util.o: src/util.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<;

objs/config.o: src/config.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<;

objs/usage.o: src/usage.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<;

objs/logging.o: src/logging.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<;

objs/handler.o: src/handler.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<;

objs/reaper.o: src/reaper.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<;

objs/child.o: src/child.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<;

objs/main.o: main.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<;

cntrinit: $(OBJS)
	$(CC) $(CFLAGS) $(STATIC_LDFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	@rm -rf objs cntrinit
