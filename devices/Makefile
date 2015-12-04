CC=gcc
CFLAGS=-Wall -I../inc -I..

OBJS = \
    dht.o \
    hcsr04.o \
    ds_therm.o \
    shr_piso.o \
    nrf_hal.o

.PHONY: all clean

all: $(OBJS)

clean:
	rm -f $(OBJS)

../config.h=../config.h
../inc/librasp/common.h=../inc/librasp/common.h
../inc/librasp/spi.h=../inc/librasp/spi.h
../inc/librasp/devices/dht.h=../inc/librasp/devices/dht.h
../inc/librasp/devices/hcsr04.h=../inc/librasp/devices/hcsr04.h
../inc/librasp/devices/ds_therm.h=../inc/librasp/devices/ds_therm.h
../inc/librasp/devices/shr_piso.h=../inc/librasp/devices/shr_piso.h
../inc/librasp/devices/nrf_hal.h=../inc/librasp/devices/nrf_hal.h $(../inc/librasp/spi.h)
../common.h=../common.h $(../config.h) $(../inc/librasp/common.h)

dht.o: $(../inc/librasp/devices/dht.h)
hcsr04.o: $(../inc/librasp/devices/hcsr04.h)
ds_therm.o: $(../inc/librasp/devices/ds_therm.h)
shr_piso.o: $(../inc/librasp/devices/shr_piso.h)
nrf_hal.o: $(../inc/librasp/devices/nrf_hal.h)

%.o: %.c $(../common.h)
	$(CC) -c $(CFLAGS) $< -o $@
