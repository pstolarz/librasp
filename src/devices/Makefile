.PHONY: all clean

CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -I../inc -I..

AR = $(CROSS_COMPILE)ar
MAKEDEP = $(CROSS_COMPILE)gcc $(CFLAGS) $< -MM -MT "$@ $*.o" -MF $@

OBJS = \
    dht.o \
    hcsr04.o \
    ds_therm.o \
    shr_piso.o \
    nrf_hal.o

all: devices.a

clean:
	$(RM) devices.a $(OBJS) $(OBJS:.o=.d)

devices.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.c
	$(MAKEDEP)

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS:.o=.d)
endif
