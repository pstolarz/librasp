CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -I../../inc

EXAMPLES = \
    nrf_rx \
    nrf_tx \
    nrf_tx_car \
    nrf_scan

.PHONY: all clean FORCE

all: $(EXAMPLES)

clean:
	$(RM) $(EXAMPLES)

../../librasp.a: FORCE
	$(MAKE) -C../..

%: %.c ../../librasp.a
	$(CC) $(CFLAGS) $^ -o $@
