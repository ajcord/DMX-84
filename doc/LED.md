DMX-84 Indicator LED
====================

Overview
--------

The DMX-84 reference hardware includes an indicator LED that can be used to
display system status to the user through a series of flashes.

The LED flash pattern is determined based on the current status flags. Each
pattern has a certain priority level that is used to determine which pattern
should be used.

Flash patterns
--------------

The various flash patterns are listed below in order of the probability of
appearance. Each table indicates the LED state in 0.1-second intervals, where
**X** means the LED is on and **\_** means the LED is off.

### Normal mode

 X | X | X | X | X | X | X | X | X | X | _ | _ | _ | _ | _
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---

When operating normally, the LED repeatedly flashes on for 1 second and off
for 0.5 seconds. Known as the heartbeat pattern, this indicates to the user
that the system is running and is not stuck in an excessively long operation
or an infinite loop. This is the default pattern and is the lowest priority.

### Error mode

 X | _ | X | _
---|---|---|---

When the system encounters an error, it immediately begins flashing on and
off in 0.2-second cycles for as long as the error status flag is set.
This is the second highest priority.

### Auto shutdown mode

 X | _ | X | _ | X | _ | _ | _ | X | X | X | _ | X | X | X | _ | X | X | X | _ | _ | _ | X | _ | X | _ | X | _ | _ | _ | _ | _
---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---

If the system has not received a command in 5 hours and 59 minutes, it will
send a warning to the calculator and will also begin flashing the LED in a
Morse code "SOS" pattern: `...---...`. This equates to three short pulses
of 0.1 seconds each, three long pulses of 0.3 seconds each, three more
short pulses, and 0.5 seconds of pausing before continuing the cycle. This
pattern is active for as long as the auto shutdown warning flag is set, and is
the second lowest priority.

### Debug mode

 X | _ | _ | _ | _ | _ | _ | _ | _ | _
---|---|---|---|---|---|---|---|---|---

If the system encounters a debug command from the calculator (`0xDB`), it
will flash the LED on for 0.1 seconds and off for 0.9 seconds. Every time the
debug command is received, the debug status flag is toggled. This pattern is
active for as long as the debug flag is set, and is the highest priority.