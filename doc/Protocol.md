DATA Project Protocol
=====================

*Protocol version 0.1*

Overview
--------

The DATA Project uses a simple communication protocol wrapped in the
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




