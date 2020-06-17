.PHONY: all clean librasp

LIBRASP_DIR=../../src
CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -I$(LIBRASP_DIR)/inc

EXAMPLES = \
    nrf_rx \
    nrf_tx \
    nrf_tx_car \
    nrf_scan

all: librasp $(EXAMPLES)

clean:
	$(RM) $(EXAMPLES)

librasp:
	$(MAKE) -C$(LIBRASP_DIR)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ -L$(LIBRASP_DIR) -lrasp
