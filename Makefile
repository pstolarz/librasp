CC=gcc
CFLAGS=-Wall -I./inc
MAKEDEP=gcc $(CFLAGS) $< -MM -MT "$@ $*.o" -MF $@

OBJS = \
    common.o \
    gpio.o \
    clock.o \
    spi.o \
    w1.o

.PHONY: all clean examples FORCE

all: librasp.a

clean:
	rm -f librasp.a $(OBJS) $(OBJS:.o=.d)
	$(MAKE) -C./devices clean
	$(MAKE) -C./examples clean

examples:
	$(MAKE) -C./examples

./devices/devices.a: FORCE
	$(MAKE) -C./devices

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

librasp.a: $(OBJS) ./devices/devices.a
	ar rcs $@ $(OBJS) ./devices/*.o

%.d: %.c
	$(MAKEDEP)

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS:.o=.d)
endif
