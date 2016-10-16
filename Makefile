CC=gcc
CFLAGS=-Wall -I./inc
MAKEDEP=gcc $(CFLAGS) $< -MM -MT "$@ $*.o" -MF $@

OBJS = \
    common.o \
    gpio.o \
    clock.o \
    spi.o \
    w1.o

.PHONY: all clean examples w1_patch ctags FORCE

all: librasp.a

clean:
	rm -f librasp.a $(OBJS) $(OBJS:.o=.d) tags
	$(MAKE) -C./devices clean
	$(MAKE) -C./examples clean

examples:
	$(MAKE) -C./examples

w1_patch:
	@if [ "${KERNEL_SRC}x" = "x" ]; then \
	  echo "ERROR: KERNEL_SRC must be set to the patched kernel source directory"; \
	  exit 1; \
	fi;
	patch -p1 -u -d ${KERNEL_SRC} <./kernel/w1_netlink.patch

ctags:
	ctags -R --c-kinds=+px --c++-kinds=+px .

./devices/devices.a: FORCE
	$(MAKE) -C./devices

librasp.a: $(OBJS) ./devices/devices.a
	ar rcs $@ $(OBJS) ./devices/*.o

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.c
	$(MAKEDEP)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),w1_patch)
ifneq ($(MAKECMDGOALS),ctags)
-include $(OBJS:.o=.d)
endif
endif
endif
