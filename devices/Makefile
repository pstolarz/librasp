CC=gcc
CFLAGS=-Wall -I../inc -I..

OBJS = \
    dht.o \
    hcsr04.o \
    ds_therm.o \
    shr_piso.o \
    nrf_hal.o

.PHONY: all clean

all: devices.a

clean:
	rm -f devices.a $(OBJS)

# headers dependencies
../inc/librasp/devices/dht.h: ../inc/librasp/gpio.h ../inc/librasp/clock.h
../inc/librasp/devices/ds_therm.h: ../inc/librasp/w1.h
../inc/librasp/devices/hcsr04.h: ../inc/librasp/gpio.h ../inc/librasp/clock.h
../inc/librasp/devices/nrf_hal.h: ../inc/librasp/spi.h
../inc/librasp/devices/shr_piso.h: ../inc/librasp/gpio.h

%.h:
	@touch -c $@

dht.o: ../common.h ../inc/librasp/devices/dht.h
ds_therm.o: ../common.h ../inc/librasp/devices/ds_therm.h
hcsr04.o: ../common.h ../inc/librasp/devices/hcsr04.h
nrf_hal.o: ../inc/librasp/devices/nrf_hal.h
shr_piso.o: ../common.h ../inc/librasp/devices/shr_piso.h

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

devices.a: $(OBJS)
	ar rcs $@ $(OBJS)
