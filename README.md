dmx-84
======

A DMX-512 driver for the TI-84

Overview
--------
NOTE: This branch is only provided for reference. It is no longer being developed and is probably not functional.

This is an ~~in-development~~ outdated driver for TI-84 calculators to run DMX equipment. It is written in z80 assembly and is formatted as an axiom so it can be used in Axe programs. The driver requires a custom-built adapter to convert the signal from the calculator to a valid DMX signal.

How It Works
------------
The driver controls the link port to calculate and send bits at 250 kHz to the transceiver. The transceiver is powered by USB mini from the calculator. It then converts the link port input to a differential DMX output and sends it through the DMX out port. The driver is initiated by StartDMX() and sends a DMX packet twice every second. The driver is suspended by the StopDMX command. Note: The driver only sends 256 channels due to hardware limitations.

This project is not usable. Do not attempt to compile the driver or build the adapter. I am not responsible for any damage that may occur due to the use of this program or the adapter.
