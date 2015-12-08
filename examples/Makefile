CC=gcc
CFLAGS=-Wall -I../inc

EXAMPS = \
    blink \
    usleep_stc \
    piso \
    w1_list \
    dsth_list \
    dht_probe \
    hcsr_probe \
    nrf24

.PHONY: all clean nrf24

all: $(EXAMPS)

clean:
	rm -f $(EXAMPS)
	$(MAKE) -C./nrf24 clean

nrf24:
	$(MAKE) -C./nrf24

../librasp.a:
	$(MAKE) -C.. librasp.a

%: %.c ../librasp.a
	$(CC) $(CFLAGS) -o $@ $^
