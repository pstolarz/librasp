This directory contains a patch file for Linux kernel module `wire`, enabling
parasite mode powering of w1 slaves accessible via the w1-netlink interface.

Technically the patch extends w1-netlink protocol by a new command:
`W1_CMD_WRITE_PULLUP` allowing writing to the bus with a subsequent strong
pull-up time when a direct voltage source is provided to the bus. During the
pull-up there is no activity on the bus until the end of the pull-up time.

The patch should be applied w/o any problems on kernels from 3.15 to 4.x.
To patch the kernel issue:

    KERNEL_SRC=<kernel-src-dir> make w1_patch

from the main project directory. `KERNEL_SRC` needs to point to the root of
patched kernel source directory.
