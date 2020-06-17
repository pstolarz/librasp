.PHONY: all clean librasp examples w1_patch

all: librasp

clean:
	$(MAKE) -C./src clean
	$(MAKE) -C./examples clean

librasp:
	$(MAKE) -C./src

examples:
	$(MAKE) -C./examples

w1_patch:
	@if [ "${KERNEL_SRC}x" = "x" ]; then \
	  echo "ERROR: KERNEL_SRC must be set to the patched kernel source directory"; \
	  exit 1; \
	fi;
	patch -p1 -u -d ${KERNEL_SRC} <./src/kernel/w1_netlink.patch
