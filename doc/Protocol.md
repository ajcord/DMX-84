dmx-84 Protocol
=====================

*Protocol version 0.3*

Overview
--------

dmx-84 uses a simple communication protocol wrapped in the
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
the DATA Project protocol is exposed at the transport layer.

Each packet begins with a single byte specifying the message type.
Depending on the type, data may follow. Each message type denotes
the format of the data that follows (if any), including whether
there must be a data length specified.

### Session Layer

The DATA Project protocol also handles initiation and termination
of communication sessions. All calculator packets are rejected by
the Arduino until it receives a ready check packet. All
non-restricted commands are then accepted until it receives either a
shutdown command, at which time it will stop listening for messages,
or a no-op command, at which time it will allow executing restricted
commands for 1 second.

Layers above the session layer are not in the scope of the
DATA Project protocol.


Implementation
--------------

### External packet structure

Packets are wrapped by the TI Link Protocol, documented in the
[TI Link Guide](http://merthsoft.com/linkguide/ti83+/packet.html).
Packets are generally sent as bulk data transfers, command ID 0x15,
with the exception of the request to send, acknowledge,
negative acknowledge, and error packets.

### Internal packet structure

Inside the wrapper packet, if the wrapper is a bulk data packet, then
an internal packet exists and contains at least one command byte,
followed by data if specified by the command. There are three types
of internal packets: single command packets, fixed width packets,
and variable width packets. Single command packets contain a single
byte: the command byte. Fixed width packets contain a single command
byte and a fixed number of data bytes that must all be present.
Variable width packets contain a single command byte, one or more
length bytes, and a number of data bytes specified by the length.

### Command bytes

Command | Name | Description | Following Data
--------|------|-------------|---------------
`0x00` | Heartbeat | Polls the transmitter, which responds with `0x00`, but doesn't enable restricted commands. | None
`0x01` | Enable restricted mode | Polls the transmitter, which responds with `0x01`, and enables restricted commands. | None
`0x10`, `0x11` | Set single channel | Sets a single channel. Last bit of command denotes upper bit of channel number. | Next byte is LSB of channel number. Third byte is new channel value.
`0x20`, `0x21` | Set 256 channels | Sets 256 channels at once. | Next 256 bytes are channel values. Sets channels 0-255 if command was `0x20` or 256-511 if command was `0x21`.
`0x22` | Set 512 channels | Sets 512 channels at once. | Next 512 bytes are channel values.





