The DATA Project
======

*A DMX-512 driver for the TI-84*

Overview
--------
dmx-84 is now the DATA Project, which stands for DMX-Arduino-TI Adapter. The entire project has been redesigned from the ground up to offload the DMX transmission to an Arduino controller.

This is an in-development hardware and software driver for TI-84 calculators to run DMX equipment. It is written in z80 assembly and is formatted as an axiom so it can be used in Axe programs. The software driver may in the future be written to be compatible with normal z80 programs. The driver requires an Arduino-based adapter to transmit the valid DMX signal.

Keep in mind that some aspects of both the hardware and the software are still being analyzed. For example, the current design does not include isolation between the DMX port and the Arduino, but this feature is being considered to ensure the safety of the Arduino and the calculator.

NOTE: This project is still in active development and is not yet usable. Do not attempt to compile the driver or build the adapter. I am not responsible for any damage that may occur due to the use or misuse of this driver or the adapter.

How It Works
------------

### The software
The software driver is an implementation of an application-specific protocol called DATAlink. It is a two-wire serial protocol designed to operate at 9600 bps so that it is compatible with the TI-8x line of calculators. The protocol was designed to maximize the efficiency of transmissions to minimize average transmission time. Simple calculations show that a full 512-channel transmission at 9600 bps would take just under half a second, which is unacceptable in a real-world context. Therefore, the default bulk packet size will be 128 channels, although this can be negotiated by the software driver up to 512 channels. It is recommended to set the bulk packet size to be as small as possible.

DATAlink also has many features besides bulk packet transfer that are designed for specific DMX use cases in order to minimize transmission time. These include setting a single channel to a specified value, setting all channels to a specified value, fading up or down a single channel or all channels to a specified value, and many more. The exact feature set is still under consideration. These can be used in preference to bulk transfers to keep transmissions short. Bulk transfers should only be used in cases where it would make more sense than setting channels individually, such as when loading channel values from a saved cue.

###The hardware
The hardware driver is an Arduino Pro Mini 328 - 5V/16MHz produced by Sparkfun. This board was selected because its small feature set is perfect for this project, especially because it can be powered by USB Mini B from the calculator (or an external USB power supply). It was also selected because of its small size and relatively low price. The Arduino handles the DATAlink input and stores the channel values in memory. It transmits DMX packets at 250 kbaud over the TX pin. Between the Arduino and the DMX Out port is a transceiver that converts the 5/0V signal to a differential +/-5V signal.

The Arduino will connect to the calculator through a combination of a 2.5mm audio jack for the serial data and a USB Mini B to FTDI adapter for power and firmware updates. The Arduino will transmit DMX packets over a 5-pin XLR female jack. The 2.5mm jack shunts will probably also be connected to two spare pins on the Arduino so that the Arduino can check whether the calculator is disconnected and can perform any necessary actions based on that.

The adapter will most likely be housed in a small aluminum enclosure, painted black for theater use.
