CC=gcc
CFLAGS=-Wall -I./inc

OBJS = \
    common.o \
    gpio.o \
    clock.o \
    spi.o \
    w1.o

.PHONY: all clean devices

all: librasp.a

clean:
	rm -f librasp.a $(OBJS)
	$(MAKE) -C./devices clean
	$(MAKE) -C./examples clean

devices:
	$(MAKE) -C./devices

config.h=config.h
inc/librasp/bcm_platform.h=inc/librasp/bcm_platform.h
inc/librasp/common.h=inc/librasp/common.h
inc/librasp/clock.h=inc/librasp/clock.h
inc/librasp/gpio.h=inc/librasp/gpio.h
inc/librasp/spi.h=inc/librasp/spi.h
inc/librasp/w1.h=inc/librasp/w1.h
common.h=common.h $(config.h) $(inc/librasp/common.h)
w1_netlink.h=w1_netlink.h

common.o: $(inc/librasp/bcm_platform.h)
clock.o: $(inc/librasp/clock.h) $(inc/librasp/bcm_platform.h)
gpio.o: $(inc/librasp/gpio.h) $(inc/librasp/bcm_platform.h)
spi.o: $(inc/librasp/spi.h)
w1.o: $(inc/librasp/w1.h) $(w1_netlink.h)

%.o: %.c $(common.h)
	$(CC) -c $(CFLAGS) $< -o $@

librasp.a: $(OBJS) devices
	ar rcs $@ $(OBJS) ./devices/*.o
