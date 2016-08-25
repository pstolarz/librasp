CC=gcc
CFLAGS=-Wall -I./inc

OBJS = \
    common.o \
    gpio.o \
    clock.o \
    spi.o \
    w1.o

.PHONY: all clean FORCE

all: librasp.a

clean:
	rm -f librasp.a $(OBJS)
	$(MAKE) -C./devices clean
	$(MAKE) -C./examples clean

./devices/devices.a: FORCE
	$(MAKE) -C./devices

# headers dependencies
common.h: config.h inc/librasp/common.h
inc/librasp/clock.h: inc/librasp/common.h inc/librasp/bcm_platform.h
inc/librasp/gpio.h: inc/librasp/common.h inc/librasp/bcm_platform.h
inc/librasp/spi.h: inc/librasp/common.h
inc/librasp/w1.h: inc/librasp/common.h

%.h:
	@touch -c $@

clock.o: common.h inc/librasp/clock.h
common.o: common.h inc/librasp/bcm_platform.h
gpio.o: common.h inc/librasp/gpio.h
spi.o: common.h inc/librasp/spi.h
w1.o: common.h w1_netlink.h inc/librasp/w1.h

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

librasp.a: $(OBJS) ./devices/devices.a
	ar rcs $@ $(OBJS) ./devices/*.o
