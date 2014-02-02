#include <DmxSimple.h>
/*
void setup() {
  DmxSimple.maxChannel(4);
  DmxSimple.write(1, 255);
  DmxSimple.write(2, 127);
  DmxSimple.write(3, 63);
  DmxSimple.write(4, 31);
}

void loop() {
}
*/

int maxChannels, brightness, percentBrightness;

void setup() {
  Serial.begin(9600);
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  Serial.println("Max channels? (1-512)");
  int startTime = millis();
  while (maxChannels == 0 && (millis() - startTime) < 5000) {
    maxChannels = (Serial.parseInt() - 1) % 512 + 1;
  }
  if (maxChannels == 0) maxChannels = 512;
  Serial.println("Brightness? (1-100)");
  startTime = millis();
  while (brightness == 0 && (millis() - startTime) < 5000) {
    brightness = (Serial.parseInt() - 1) % 256 + 1;
  }
  if (brightness == 0) brightness = 255;
  Serial.print("Testing ");
  Serial.print(maxChannels);
  Serial.print(" channels at ");
  brightness = 255;
  Serial.print(brightness);
  Serial.println("...");
  DmxSimple.usePin(10);
  DmxSimple.maxChannel(maxChannels);
}

void loop() {
  for (int channel = 1; channel <= maxChannels; channel++) {
    Serial.print("Testing channel ");
    Serial.print(channel);
    Serial.println("...");
    DmxSimple.write(channel, brightness);
    delay(1000);
    DmxSimple.write(channel, 0);
  }
}
