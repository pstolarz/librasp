CC=gcc
CFLAGS=-Wall -I../../inc

EXAMPS = \
    nrf_rx \
    nrf_tx \
    nrf_tx_car \
    nrf_scan

.PHONY: all clean makelib

all: $(EXAMPS)

clean:
	rm -f $(EXAMPS)

makelib ../../librasp.a:
	$(MAKE) -C../.. librasp.a

%: %.c makelib ../../librasp.a
	@if [ "$?" != "makelib" ]; then \
	  (PS4=; set -x; $(CC) $(CFLAGS) $< -o $@ ../../librasp.a;) \
	fi
