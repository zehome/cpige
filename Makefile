CC ?= gcc
CFLAGS +=-Wall

ifeq ($(NOCONFIG),1)
	CFLAGS += -DNOCONFIG
endif

OBJS = debug.o mytime.o tool.o pool.o configlib.o cpige.o mynet.o id3.o icy.o

all: cpige

cpige: $(OBJS)
	$(CC) $(CFLAGS) $(CLIBS) $(OBJS) -o cpige

clean:
	rm -rf *.o cpige *.exe
