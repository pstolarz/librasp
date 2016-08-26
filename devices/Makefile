CC=gcc
CFLAGS=-Wall -I../inc -I..
MAKEDEP=gcc $(CFLAGS) $< -MM -MT "$@ $*.o" -MF $@

OBJS = \
    dht.o \
    hcsr04.o \
    ds_therm.o \
    shr_piso.o \
    nrf_hal.o

.PHONY: all clean

all: devices.a

clean:
	rm -f devices.a $(OBJS) $(OBJS:.o=.d)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

devices.a: $(OBJS)
	ar rcs $@ $(OBJS)

%.d: %.c
	$(MAKEDEP)

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS:.o=.d)
endif
