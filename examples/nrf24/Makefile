CC=gcc
CFLAGS=-Wall -I../../inc

EXAMPS = \
    nrf_rx \
    nrf_tx \
    nrf_tx_car \
    nrf_scan

.PHONY: all clean FORCE

all: $(EXAMPS)

clean:
	rm -f $(EXAMPS)

../../librasp.a: FORCE
	$(MAKE) -C../.. librasp.a

%: %.c ../../librasp.a
	$(CC) $(CFLAGS) $^ -o $@
