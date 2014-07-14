DMX-84 Indicator LED
====================

Overview
--------

The DMX-84 reference hardware includes an indicator LED that can be used to
display system status to the user through a series of flashes.

Flash patterns
--------------

### Normal mode

When operating normally, the LED repeatedly flashes on for 1 second and off
for 0.5 seconds. Known as the heartbeat pattern, this indicates to the user
that the system is running and is not stuck in an excessively long operation
or an infinite loop.

### Error mode

When the system encounters an error, it immediately begins flashing on and
off in 0.1-second intervals. Once the error status has been cleared, which
occurs when the current error flags are requested from the calculator, then
the LED returns to its previous flashing pattern.

### Auto shutdown mode

If the system has not received a command in 5 hours and 59 minutes, it will
send a warning to the calculator and will also begin flashing the LED in a
Morse code "SOS" pattern: `...---...`. This equates to three short pulses
of 0.1 seconds each, three longer pulses of 0.3 seconds each, three more
short pulses, and 0.5 seconds of pausing before continuing the cycle. If
auto shutdown is cancelled by receiving another command before the timeout,
then the LED will revert to the previous flash pattern.

### Debug mode

If the system encounters a debug command from the calculator (`0xDB`), it
will flash the LED on for 0.1 seconds and off for 0.9 seconds. If it receives
another debug command, it will return to its previous flashing mode.