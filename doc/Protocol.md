DMX-84 Protocol
=====================

*Protocol version 0.3.1*

Overview
--------

DMX-84 uses a simple communication protocol wrapped in the
TI graphing calculator two-wire link protocol. This communication
protocol allows the calculator to communicate with the Arduino to
send commands, update channel data, etc. It is designed with maximum
transfer speed and data efficiency in mind to minimize the drawbacks
of limited processing power and memory space.


Layers
------

### Physical Layer

Packets are sent over the standard TI graphing calculator 2-wire
link cable. Other cable types are not currently supported; however,
USB support may be added in the future. Details of the physical layer
are available in the
[TI Link Guide](http://merthsoft.com/linkguide/hardware.html).

### Data Link Layer

The Data Link layer is implemented as the standard TI-83/84 link
protocol. The TI-84 OS implementation is the reference implementation
for the development of this project. See the
[TI Link Guide](http://merthsoft.com/linkguide/ti83+/packet.html)
for details on the TI Link Protocol.

### Network Layer

The TI Link Protocol does not provide networking capabilities
because only two devices can be connected at once. It does, however,
provide a method for identifying machine types. See the
[TI Link Guide](http://merthsoft.com/linkguide/ti83+/packet.html)
for a list of machine IDs.

### Transport Layer

Having been wrapped by the TI Link Protocol for the lower layers,
the DMX-84 protocol is exposed at the transport layer.

Each packet begins with a single byte specifying the message type.
Depending on the type, data may follow. Each message type denotes
the format of the data that follows (if any), including whether
there must be a data length specified.

### Session Layer

The DMX-84 protocol also handles initiation and termination
of communication sessions. All calculator packets are rejected by
the Arduino until it receives a ready check packet. All
non-restricted commands are then accepted until it receives either a
shutdown command, at which time it will stop listening for messages,
or a no-op command, at which time it will allow executing restricted
commands for 1 second.

Layers above the session layer are not in the scope of the
DMX-84 protocol.


Implementation
--------------

### External packet structure

Packets are wrapped by the TI Link Protocol, documented in the
[TI Link Guide](http://merthsoft.com/linkguide/ti83+/packet.html).
Packets are generally sent as bulk data transfers, command ID 0x15,
with the exception of the clear to send, ready check, acknowledge,
negative acknowledge, and error packets.

### Internal packet structure

If the received TI packet is marked as bulk data, then the bulk data
it contains is a DMX-84 command. A command consists of a single command byte
followed by data if specified by the command. There are three types
of DMX-84 commands: single commands, fixed width commands,
and variable width commands. Single commands naturally contain one
byte: the command byte. Fixed width commands contain one command
byte and a fixed number of data bytes or parameters. Variable width packets
contain one command byte, one or more parameters if applicable, one
length byte, and a number of data bytes specified by the length.

### Communication flow

When the Arduino is finished booting, it sends a clear-to-send packet
to the calculator. The calculator should attempt to receive and acknowledge
this if it is known that the Arduino has started booting after the
calculator program has started (for example, if the calculator is
providing power to the Arduino).

Next, the calculator must send a ready check packet and receive
acknowledgment before any further communication can occur.

The calculator can then send bulk data packets following the format
specified below. Every bulk data packet is acknowledged by the Arduino.
If a command byte will result in a reply from the Arduino,
the calculator must be ready to receive that reply to prevent a link error.
Note that the Arduino's reply always begins with the command byte it
is responding to. This means that to receive a reply, the calculator must
be listening for one byte more than the expected reply length. For example,
the Uptime command replies with 4 bytes, but the calculator must receive
5 bytes to accommodate the extra command byte.

In order to initiate shut down or reset, the calculator must first
put the Arduino in restricted mode by sending `0x01`. This will enable
access to the shut down and reset commands for one second before
reverting to regular mode. When shutting down or resetting, the Arduino
will send an end of transmission packet immediately before it enters
sleep mode or resets.

If no commands are received within 5 hours and 59 minutes, the Arduino
will send a warning packet to the calculator. This is the only unsolicited
message the Arduino can send. If the Arduino doesn't receive a command
within 60 seconds, it will automatically shut down to save the
calculator's batteries in case the calculator is powering the Arduino.

### Command bytes

Command | Name | Description | Following Data
--------|------|-------------|---------------
`0x00` | Heartbeat | Polls the Arduino, which responds with `0x00`. | None
`0x01` | Enable restricted mode | Polls the Arduino, which responds with `0x01`, and enables restricted commands. | None
`0x10`, `0x11` | Set single channel | Sets a single channel. Last bit of command denotes upper bit of channel number. | Next byte is LSB of channel number. Third byte is new channel value.
`0x12`, `0x13` | Increment single channel | Increments a single channel by 1. Last bit of command denotes upper bit of channel number. | Next byte is LSB of channel number.
`0x14`, `0x15` | Decrement single channel | Decrements a single channel by 1. Last bit of command denotes upper bit of channel number. | Next byte is LSB of channel number.
`0x16`, `0x17` | Increment single channel by value | Increments a single channel by the specified amount. Last bit of command denotes upper bit of channel number. | Next byte is amount to increment.
`0x18`, `0x19` | Decrement single channel by value | Decrements a single channel by the specified amount. Last bit of command denotes upper bit of channel number. | Next byte is amount to decrement.
`0x20`, `0x21` | Set 256 channels | Sets 256 channels at once. Last bit of command denotes upper bit of channel block. | Next 256 bytes are channel values. Sets channels 0-255 if command was `0x20` or 256-511 if command was `0x21`.
`0x22`, `0x23` | Set block of channels | Sets a block of channels at once. Last bit of command denotes upper bit of start channel. | Next byte is LSB of start channel number. Third byte is number of channels to set (X). Next X bytes are new channel values.
`0x24` | Increment all channels by value | Increments all channel by a value. | Next byte is amount to increment.
`0x25` | Decrement all channels by value | Decrements all channel by a value. | Next byte is amount to increment.
`0x26` | Set all channels to value | Sets all channel to the same value. | Next byte is new value.
`0x27` | Set 512 channels | Sets 512 channels at once. | Next 512 bytes are channel values.
`0x30`, `0x31` | Copy 256 channels | Copies 256 channels between 0-255 and 256-511. Last bit of command denotes direction: `0x30` copies 256-511 to 0-255, and `0x31` copies the other direction. | None
`0x32` | Exchange 256 channels | Exchanges 256 channels between 0-255 and 256-511. | None
`0x40`, `0x41` | Get single channel value | Gets the value of a single channel. Last bit of command denotes upper bit of channel number. | Next byte is LSB of channel number.
`0x42` | Get all channel values | Gets the value of all channels. | None
`0x5A` | Toggle LED | Toggles the LED (for debugging). | None
`0xE0` | Stop DMX | Stops transmitting DMX. | None
`0xE1` | Start DMX | Starts transmitting DMX. Note that DMX is set to transmit upon power up. | None
`0xE2`, `0xE3` | Set max channels | Sets the maximum number of channels to transmit. Last bit of command denotes upper bit of channel number. Note that a value of 0 sets the max channels to 512. | Next byte is LSB of number of channels.
`0xE4` | Start digital blackout | Transmits 0 for all channels, keeping the old values in memory. | None
`0xE5` | Stop digital blackout | Transmits channels as they were before the digital blackout. | None
`0xF0` | Shut down | Shuts down the Arduino, which responds with `0xF0` and then EOT. Only works in restricted mode. | None
`0xF1` | Reset | Soft-resets the Arduino, which responds with `0xF1` and then EOT. Only works in restricted mode. | None
`0xF8` | Status request | Responds with the current status flags. | None
`0xF9` | Error request | Responds with the current error flags and then resets them. | None
`0xFA` | Version request | Responds with the 3-byte protocol and firmware versions, little-Endian style. | None
`0xFB` | Protocol version request | Responds with the 3-byte protocol version, little-Endian style. | None
`0xFC` | Firmware version request | Responds with the 3-byte firmware version, little-Endian style. | None
`0xFD` | Temperature request | Responds with the approximate 4-byte core temperature in millidegrees Celsius, little-Endian style. | None
`0xFE` | Uptime request | Responds with the 4-byte uptime in milliseconds, little-Endian style. | None
