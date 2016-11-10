CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -I../inc

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

.PHONY: all clean nrf24 FORCE

all: $(EXAMPLES) nrf24

clean:
	$(RM) $(EXAMPLES)
	$(MAKE) -C./nrf24 clean

nrf24:
	$(MAKE) -C./nrf24

../librasp.a: FORCE
	$(MAKE) -C..

%: %.c ../librasp.a
	$(CC) $(CFLAGS) $^ -o $@
