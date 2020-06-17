.PHONY: all clean devices

CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -I./inc

AR = $(CROSS_COMPILE)ar
MAKEDEP = $(CROSS_COMPILE)gcc $(CFLAGS) $< -MM -MT "$@ $*.o" -MF $@

OBJS = \
    common.o \
    gpio.o \
    clock.o \
    spi.o \
    w1.o

all: librasp.a

clean:
	$(RM) librasp.a $(OBJS) $(OBJS:.o=.d)
	$(MAKE) -C./devices clean

devices:
	$(MAKE) -C./devices

librasp.a: devices $(OBJS)
	$(AR) rcs $@ $(OBJS) ./devices/*.o

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.c
	$(MAKEDEP)

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS:.o=.d)
endif
