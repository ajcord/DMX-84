DMX-84
======

*A DMX-512 driver for the TI-84*

Overview
--------
DMX-84 is a hardware and software driver for TI-84 calculators to run DMX
equipment such as DJ and theater lights. It is currently written as an Axe
include program, but it may be rewritten as an Axiom in a later version. The
driver requires an Arduino-based adapter to transmit DMX.

Disclaimer: I am not responsible for any damage that may occur due to the use
or misuse of the hardware and/or software specified in this repository.

How It Works
------------

### The software
The software driver on the calculator is an implementation of an
application-specific protocol. The protocol is designed to be as lightweight as
possible to minimize transmission time. To accomplish this, messages consist of
a single command byte followed by optional parameters, wrapped in a standard TI
link packet. For details on the protocol, see /doc/Protocol.md.

### The hardware
The hardware driver is an Arduino Pro Mini 328 - 5V/16MHz produced by Sparkfun.
I selected this board because its small feature set is perfect for this
project, especially because it can be powered by USB Mini B from the calculator
(or an external USB power supply). I also selected it because of its small size
and relatively low price. The Arduino handles the calculator input and stores
the channel values in memory. It transmits DMX packets at 250 kbaud using a
slightly modified version of the DmxSimple library. Between the Arduino and the
DMX Out jack is a transceiver that converts the +5/0V signal from the Arduino
pin to a differential +5/-5V signal.

The adapter connects to the calculator through a combination of a 2.5mm audio
jack for the serial data and a USB Mini B to FTDI adapter for power, serial
debugging, and firmware updates. The Arduino can also receive power from any
normal USB power supply, such as a computer or even a phone charger.

The Arduino transmits DMX packets over a 5-pin XLR female jack. An LED mounted
on the enclosure currently serves as a power indicator, although it can be
controlled in software for any purpose. All of the hardware is housed in a
small aluminum enclosure, painted black for theater use.


