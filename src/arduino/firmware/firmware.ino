/* This program allows you to set DMX channels over the serial port.
**
** After uploading to Arduino, switch to Serial Monitor and set the baud rate
** to 9600.
**
** For more details, and compatible Processing sketch,
** visit http://code.google.com/p/tinkerit/wiki/SerialToDmx
**
** Help and support: http://groups.google.com/group/dmxsimple       */

#include <DmxSimple.h>

void setup() {
  Serial.begin(9600);
  Serial.println("Ready!");
}

int value = 0;
int channel;

void loop() {
  int b;

  while(!Serial.available());
  b = Serial.read();
  if (b == 17) {
    while(!Serial.available());
    channel = Serial.read();
    while(!Serial.available());
    value = Serial.read();
    DmxSimple.write(channel, value);
    value = 0;
  }
}
