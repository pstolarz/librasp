CC=gcc
CFLAGS=-Wall -I../inc

EXAMPS = \
    blink \
    usleep_stc \
    piso \
    w1_list \
    dsth_list \
    dht_probe \
    hcsr_probe

.PHONY: all clean

all: $(EXAMPS)

clean:
	rm -f $(EXAMPS)

../librasp.a:
	$(MAKE) -C.. librasp.a

%: %.c ../librasp.a
	$(CC) $(CFLAGS) -o $@ $^
