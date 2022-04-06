CC ?= gcc
CFLAGS +=-Wall

ifeq ($(NOCONFIG),1)
	CFLAGS += -DNOCONFIG
endif

OBJS = debug.o mytime.o tool.o pool.o configlib.o cpige.o mynet.o id3.o icy.o

all: cpige

clean:
	rm -rf *.o cpige *.exe

static:
	docker run --rm -it -v $(shell pwd):$(shell pwd) -w $(shell pwd) alpine:latest ./build_alpine.sh

cpige: $(OBJS)
	$(CC) $(CFLAGS) $(CLIBS) $(OBJS) -static -o cpige


