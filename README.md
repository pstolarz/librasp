librasp
=======

Raspberry Pi library for GPIO, 1-Wire, SPI, System Time Counter (STC) access.
As an example of usage there have been provided APIs for handling various
popular devices like wireless nRF24L01 transceiver, DHT temperature sensors
(DHT11/22), Dallas family of temperature sensors (e.g. DS18B20, DS1822, DS1825),
HC-SR04 distance sensor, PISO shift register...

API specification contained in the library headers (see `./inc/librasp`).  For
examples look in `./examples`.

Expect more sensors support in the future.

Compilation
-----------

    make

produces static library `librasp.a` which may be linked into an application.

    make examples

will compile examples in `./examples`.

1-wire and parasite powering
----------------------------

The original Linux kernel 1-wire driver (the `wire` module) doesn't provide
proper interface for parasite powering of 1-wire slaves via userland's netlink
protocol.

1. To handle this issue there has been provided a patch for the kernel allowing
   the library to fully support the parasite mode. Look in `./kernel` directory
   for more info.

2. Configure the library with `CONFIG_WRITE_PULLUP` parameter by updating
   `./config.h` while building the library.

3. RPi uses `w1-gpio` module as a 1-wire bus master driver. The driver needs
   to be configured to support bit-banged strong pull-up on the data wire,
   required for parasite powering of the connected slaves. Add the following
   line in `/boot/config.txt` to turn it on via the device-tree overlay (reboot
   is required):

    `dtoverlay=w1-gpio,pullup=on`

   NOTE: The configuration is not required when using `w1-gpio-cl` driver
   instead of `w1-gpio`.

   https://github.com/pstolarz/w1-gpio-cl

   To reach for the full power of the library, it is **strongly recommended**
   to use the `w1-gpio-cl` driver instead of the standard one. The primary
   benefit is support for more than one bus master simultaneously and much
   more mature parasite powering handling. See the above link for more
   details.

Refer `./examples/dsth_list*` samples for details on probing the parasite
powered Dallas family of temperature sensors.

License
-------

2 clause BSD license. See LICENSE file for details.
