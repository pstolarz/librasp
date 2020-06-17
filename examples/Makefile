.PHONY: all clean librasp nrf24_examples

LIBRASP_DIR=../src
CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -I$(LIBRASP_DIR)/inc

EXAMPLES = \
    blink \
    gpio_poll \
    usleep_stc \
    piso \
    w1_list \
    dsth_list \
    dsth_list2 \
    dht_probe \
    hcsr_probe

all: librasp $(EXAMPLES) nrf24_examples

clean:
	$(RM) $(EXAMPLES)
	$(MAKE) -C./nrf24 clean

nrf24_examples:
	$(MAKE) -C./nrf24

librasp:
	$(MAKE) -C$(LIBRASP_DIR)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ -L$(LIBRASP_DIR) -lrasp
