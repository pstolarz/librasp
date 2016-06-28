librasp
=======

Raspberry Pi library for GPIO, 1-Wire, SPI, System Time Counter (STC) access. As
an example of usage there have been provided APIs for handling various popular
devices like wireless nRF24L01 transceiver, DHT temperature sensors (DHT11/22),
Dallas family of temperature sensors (e.g. DS18B20, DS1822, DS1825), HC-SR04
distance sensor, PISO shift register...

API specification contained in the library headers (see `./inc/librasp`).  For
examples look in `./examples`.

Expect more sensors support in the future.

1-wire and parasite powering
----------------------------

The original Linux kernel w1 driver (the `wire` module) doesn't provide proper
interface for parasite powering of w1 slaves via userland's netlink protocol.

1. To handle this issue there has been provided a patch for the kernel allowing
   the library to fully support the parasite mode. Look in `./kernel` directory
   for more info.
2. RPi uses `w1-gpio` module as a master w1 driver. The driver need to be
   configured to support bit-banged pullup for parasite powering. Add the
   following line in `/boot/config.txt` to turn it on the device-tree:
   `dtoverlay=w1-gpio,pullup=on`. Reboot is required.
3. Configure the library with `CONFIG_WRITE_PULLUP` parameter by updating
   `./config.h` while building the library.

Refer `./examples/dsth_list*` samples for details on probing the parasite powered
Dallas family of temperature sensors.

License
-------

2 clause BSD license. See LICENSE file for details.
