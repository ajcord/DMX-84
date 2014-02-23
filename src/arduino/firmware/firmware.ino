/**
 * The DATA Project
 *
 * By Alex Cordonnier
 *
 * Arduino firmware v0.2.2
 **/

#include <DmxSimple.h>

/******************************************************************************
 * Function declarations
 ******************************************************************************/

bool updateChannel(word chan, byte val);
bool updateDMX();
bool setMaxChannels(word maxCh = 128);
void transmitDMX(bool transmit = true);
void resetStatus();
void initShutDown(bool reset = false);
//Calculator linking routines:
void waitForPacket();
void resetLines();
int par_get_wait(byte *data, unsigned long len);
void reply(byte *data, unsigned long len);
static int par_put(byte *data, unsigned long len);
static int par_get(byte *data, unsigned long len);
word checksum(byte *data, word datalength);

/******************************************************************************
 * Variable declarations
 ******************************************************************************/

//Pins:
#define LED_PIN 9
//#define DMX_ENABLE_PIN 4 //Set this pin to HIGH to enable the transceiver driver
#define DMX_OUT_PIN 10 //DMX data output

const unsigned long autoShutDown = 21600000; //Number of milliseconds to wait before shutting down (6 hours)

//Calculator communication:
#define MACHINE_ID 0x23
#define TIring 4
#define TItip 6
//#define TIwhite TIring
//#define TIred TItip
#define ERR_READ_TIMEOUT 1000
#define ERR_WRITE_TIMEOUT 2000
#define TIMEOUT 4000
#define GET_ENTER_TIMEOUT 30000
byte ACK[] = {MACHINE_ID, 0x56, 0, 0}; //Acknowledge
byte NAK[] = {MACHINE_ID, 0x36, 1, 0, 1, 1, 0}; //Skip/Exit
//byte CTS[] = {MACHINE_ID, 0x09, 0, 0}; //Clear to send
byte ERR[] = {MACHINE_ID, 0x5A, 0, 0}; //Checksum error
byte packetHead[4] = {0};
byte packetData[519] = {0}; //Input buffer for packet data from calculator, minus the 4 byte header and the checksum
byte packetChecksum[2] = {0};
bool hasReceivedHandshake = false;

bool ledState = false;

//DMX:
word maxChannels = 128;
byte DMX[512] = {0}; //Stores a byte buffer of the entire DMX universe
byte cmd = 0; //Stores the last command received
unsigned long lastCmdReceived = 0; //Stores the time the last command was received
unsigned long noOpReceived = 0; //Stores the time the last no-op was received
bool isDigitalBlackoutEnabled = false; //Stores whether the digital blackout is active
bool isDmxEnabled = true; //Stores whether the DMX output is enabled

//Administrative:
bool protectionDisabled = false; //In general, stores whether a no-op was recently received
byte lastError = 0; //Stores the most recent error code
byte currentStatus = 0; //Stores the current Arduino status
const byte PROTOCOL_VERSION = 0x01; //Stores the protocol version number. High nibble = major version, low nibble = minor version.
const byte FIRMWARE_VERSION = 0x02; //Stores the firmware version number. High nibble = major version, low nibble = minor version.

/******************************************************************************
 * Functions
 ******************************************************************************/

void setup() {
  //Set up the I/O lines
  resetLines();

  //Set up DMX
  setMaxChannels(); //Set the max channels to transmit
  DmxSimple.usePin(DMX_OUT_PIN); //Set the pin to transmit DMX on
  //pinMode(DMX_ENABLE_PIN, OUTPUT);

  //Not sure what I'll end up using the LED for. For now, it will be a power indicator.
  pinMode(LED_PIN, OUTPUT);
  ledState = true;
  digitalWrite(LED_PIN, HIGH);

  //Start serial communication with the computer
  Serial.begin(115200);
  ////Serial.setTimeout(1000);
  resetStatus();

  transmitDMX(); //Enable DMX
  Serial.println("Ready");
}

void loop() {

  bool sentWarning = false; //Stores whether the calculator has been warned
  waitForPacket();
  lastCmdReceived = millis();
  
  cmd = packetData[0];
  Serial.print("Cmd: ");
  Serial.println(cmd, HEX);
  
  switch (cmd) {
    case 0x00: {
      //No-op. Allows protected commands to be executed for one second.
      //Serial.println("Received no-op");
      protectionDisabled = true;
      //byte packet[] = {0};
      reply(0, 0); //Just echo the no-op to acknowledge it
      //free(packet);
      noOpReceived = millis();
      break;
    }
    case 0x01: {
      //Heartbeat. Same as no-op but doesn't enable protected commands.
      byte packet[] = {0};
      reply(packet, 0); //Just echo the request to acknowledge it
      //Serial.println("Sent heartbeat");
      break;
    }
    case 0x10: //acts like an OR because switch executes the code until a break
    case 0x11: {
      //Sets a single channel
      updateChannel((cmd & 1)*256 + packetData[1], packetData[2]);
      //Serial.println("Updated channel");
      break;
    }
    case 0x20:
    case 0x21: {
      //Sets 256 channels at once
      for (int i = 0; i < 256; i++) {
        DMX[(cmd & 1)*256 + i] = packetData[i + 1];
      }
      updateDMX();
      //Serial.println("Updated 256 channels");
      break;
    }
    case 0x22: {
      //Sets 512 channels at once
      for (int i = 0; i < 512; i++) {
        DMX[i] = packetData[i + 1];
      }
      updateDMX();
      //Serial.println("Updated 512 channels");
      break;
    }
    case 0x23: {
      //Sets a block of channels at once
      int len = packetData[1]; //Stores the number of channels to set, starting from 0
      for (int i = 0; i < len; i++) {
        DMX[i] = packetData[i + 2];
      }
      updateDMX();
      //Serial.print("Updated ");
      //Serial.print(len);
      //Serial.println(" channels");
      break;
    }
    case 0x24: {
      //Sets all channels to the same value
      for (int i = 0; i < 512; i++) {
        DMX[i] = packetData[1]; //Set each channel to the new value
      }
      updateDMX();
      //Serial.print("Set all channels to ");
      //Serial.println(packetData[1]);
      break;
    }
    case 0x25: {
      //Increments all channels by a value
      int newValue = 0;
      for (int i = 0; i < 512; i++) {
        newValue = DMX[i] + packetData[1];
        if (newValue <= 255) { //Prevent an overflow
          DMX[i] = newValue;
        } else {
          DMX[i] = 255; //If the value is too large, set it to 255
        }
      }
      updateDMX();
      //Serial.print("Incremented all channels by ");
      //Serial.println(packetData[1]);
      break;
    }
    case 0x26: {
      //Decrements all channels by a value
      int newValue = 0;
      for (int i = 0; i < 512; i++) {
        newValue = DMX[i] - packetData[1];
        if (newValue >= 0) { //Prevent an underflow
          DMX[i] = newValue;
        } else {
          DMX[i] = 0; //If the value is too small, set it to 0
        }
      }
      updateDMX();
      //Serial.print("Decremented all channels by ");
      //Serial.println(packetData[1]);
      break;
    }
    case 0x28: {
      //Start a digital blackout
      for (int i = 0; i < maxChannels; i++) {
        DmxSimple.write(i+1, 0); //Zero the DMX output but NOT the stored values
      }
      isDigitalBlackoutEnabled = true;
      resetStatus();
      //Serial.println("Started digital blackout");
      break;
    }
    case 0x29: {
      //End a digital blackout
      isDigitalBlackoutEnabled = false;
      updateDMX(); //Restore outputting the stored values
      resetStatus();
      //Serial.println("Ended digital blackout");
      break;
    }
    case 0x30: {
      //Copy channel data from 256-511 to 0-255
      for (int i = 0; i < 256; i++) {
        DMX[i] = DMX[i+256];
      }
      updateDMX();
      //Serial.println("Copied 256-511 to 0-255");
      break;
    }
    case 0x31: {
      //Copy channel data from 0-255 to 256-511
      for (int i = 0; i < 256; i++) {
        DMX[i+256] = DMX[i];
      }
      updateDMX();
      //Serial.println("Copied 0-255 to 256-511");
      break;
    }
    case 0x32: {
      //Exchange channel data between 0-255 and 256-511
      //FIXME: Seems to sometimes set channel 256 to 0x32. Can't seem to reproduce anymore, though.
      byte temp = 0;
      for (int i = 0; i < 256; i++) {
        //Swap the values of each low channel with its high counterpart
        temp = DMX[i];
        DMX[i] = DMX[i + 256];
        DMX[i + 256] = temp;
      }
      updateDMX();
      //Serial.println("Exchanged 0-255 with 256-511");
      break;
    }
    case 0x40:
    case 0x41: {
      //Reply with channel value for specific channel
      byte packet[] = {DMX[(cmd & 1)*256 + packetData[1]]};
      //Serial.print("Channel ");
      //Serial.print((cmd & 1)*256 + packetData[1]);
      //Serial.print(" is at ");
      //Serial.println(DMX[(cmd & 1)*256 + packetData[1]]);
      /*
      //Serial.write(value); //Reply with the channel value
      byte returnPacket[] = {MACHINE_ID, 0x15, 2, 0, 0x41, value, 0, 0};
      chksm = checksum(returnPacket + 4, 2);
      returnPacket[6] = chksm & 0xFF;
      returnPacket[7] = chksm >> 8;
      par_put(returnPacket);*/
      reply(packet, 1);
      break;
    }
    case 0x42: {
      //Reply with all channel values
      /*
      byte returnPacket[519] = {0};
      returnPacket[0] = MACHINE_ID;
      returnPacket[1] = 0x15;
      returnPacket[2] = 2;
      returnPacket[3] = 7;
      returnPacket[4] = 0x42;
      for (int i = 0; i < 512; i++) {
        returnPacket[i + 5] = DMX[i];
      }
      chksm = checksum(returnPacket + 4, 513);
      returnPacket[517] = chksm & 0xFF;
      returnPacket[518] = chksm >> 8;
      par_put(returnPacket);*/
      reply(DMX, 512);
      //Serial.begin(115200);
      Serial.println("Channel values:");
      for (int i = 0; i < 512; i++) {
        Serial.print(DMX[i]);
        Serial.print(",");
      }
      Serial.println();
      //Serial.end();
      break;
    }

    case 0x5A: {
      //Prints a newline. Used for debugging in the Serial Monitor.
      //Serial.println();
      //Now toggles the LED.
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
      break;
    }

    /* Intermediate keywords reserved for future use */

    case 0xE0: {
      //Stop transmitting DMX
      transmitDMX(false);
      //Serial.println("Stopped DMX");
      break;
    }
    case 0xE1: {
      //Start transmitting DMX
      transmitDMX(true);
      //Serial.println("Started DMX");
      break;
    }
    case 0xE2:
    case 0xE3: {
      //Set max channels to transmit
      setMaxChannels((cmd & 1)*256 + packetData[1]); //Set the max number of channels
      //Serial.print("Max channels now ");
      //Serial.println((cmd & 1)*256 + packetData[1]);
      break;
    }
    case 0xF0: {
      //Initiate safe shutdown sequence - only works directly after a no-op
      initShutDown();
      break;
    }
    case 0xF1: {
      //Initiate soft reset - only works directly after a no-op
      initShutDown(true);
      break;
    }
    case 0xF8: {
      //Reply with current status
      byte packet[] = {currentStatus};
      reply(packet, 1);
      //Serial.print("Current status: ");
      //Serial.println(currentStatus, HEX);
      break;
    }
    case 0xF9: {
      //Reply with last error code
      byte packet[] = {lastError};
      reply(packet, 1);
      //Serial.print("Last error: ");
      //Serial.println(lastError, HEX);
      break;
    }
    case 0xFA: {
      //Reply with version numbers
      byte packet[] = {PROTOCOL_VERSION, FIRMWARE_VERSION};
      reply(packet, 2);
      //Serial.print("Protocol version: ");
      //Serial.println(PROTOCOL_VERSION, HEX);
      //Serial.print("Firmware version: ");
      //Serial.println(FIRMWARE_VERSION, HEX);
      break;
    }
    default: {
      //Unknown command; set the error code
      lastError = 0x21;
      byte packet[] = {lastError};
      reply(packet, 1);
      //Serial.println("Unknown command");
      break;
    }
  }
}

/**
 * Updates a single channel in both the DMX buffer and DmxSimple
 */

bool updateChannel(word chan, byte val) {
  /* If DMX is disabled and we call DmxSimple.write, it will become re-enabled,
     so check before we call it. */
  if (isDmxEnabled) {
    DMX[chan] = val;
    DmxSimple.write(chan+1, val); //DmxSimple starts counting at 1. How strange.
    return true;
  } else {
    lastError = 2; //DMX is disabled
    return false;
  }
}

/**
 * Updates all the DmxSimple channels with the stored values in DMX[]
 */

bool updateDMX() {
  /* Prevent updating the DMX output when digital blackout is enabled or
     DMX is disabled */
  if (isDmxEnabled && !isDigitalBlackoutEnabled) {
    for (int i=0; i < maxChannels; i++) {
      DmxSimple.write(i+1, DMX[i]);
    }
    return true;
  } else {
    if (isDigitalBlackoutEnabled) lastError = 1; //Digital blackout is enabled
    if (!isDmxEnabled) lastError = 2; //DMX is disabled
    return false;
  }
}

/**
 * Sets the maximum number of channels to transmit.
 */

bool setMaxChannels(word maxCh) {
  /* Setting max channels to zero disables DMX, so prevent doing that
     accidentally */
  if (isDmxEnabled && maxCh > 0 && maxCh <= 512) {
    maxChannels = maxCh;
    DmxSimple.maxChannel(maxCh);
    return true;
  } else {
    if (maxCh <= 0) lastError = 0x20; //Invalid value
    if (!isDmxEnabled) lastError = 2; //DMX is disabled
    return false;
  }
}

/**
 * Sets the current status based on the value of flags
 */

void resetStatus() {
  if (protectionDisabled) {
    currentStatus = 3;
  } else if (!isDmxEnabled) {
    currentStatus = 2;
  } else if (isDigitalBlackoutEnabled) {
    currentStatus = 1;
  } else {
    currentStatus = 0;
  }
}

/**
 * Enables or disables DMX output. Defaults to enable.
 */

void transmitDMX(bool transmit) {
  if (transmit) {
    //digitalWrite(DMX_ENABLE_PIN, HIGH); //Enable the transceiver driver
    DmxSimple.maxChannel(maxChannels); //Start transmitting DMX from the Arduino
    isDmxEnabled = true;
  } else {
    DmxSimple.maxChannel(0); //Stop transmitting DMX from the Arduino
    //digitalWrite(DMX_ENABLE_PIN, LOW); //Disable the transceiver driver
    isDmxEnabled = false;
  }
  resetStatus();
}

/**
 * Shuts down the Arduino and initiates a soft reset. 
 * Cannot return successfully.
 */

void initShutDown(bool reset) {
  if (protectionDisabled) {
    //currentStatus = 0xFE; //Getting ready to power down

    transmitDMX(false); //Disable DMX transmission

    //currentStatus = 0xFF; //Ready for power down
    byte statusPacket[] = {MACHINE_ID, 0x15, 1, 0, 0xFF, 0xFF, 0};
    par_put(statusPacket, 7);
    byte EOT[] = {MACHINE_ID, 0x92, 0, 0}; //End of transmission
    par_put(EOT, 4);
    //Serial.println("System is going down NOW!");
    //Serial.end(); //Stop serial communication
    digitalWrite(LED_PIN, LOW);
    if (reset) asm volatile ("jmp 0"); //Reset the software
    while (true); //Go into an infinite loop
  } else {
    lastError = 3; //Last command wasn't no-op
  }
}

/**
 * Waits for a valid packet to be received and stores received data in packetData.
 */

void waitForPacket() {
  bool keepWaiting = true;
  while (keepWaiting) {
    resetLines();
    
    //Get the packet
    par_get_wait(packetHead, 4); //At first, only attempt to get 4 bytes so we know how long the packet is
    unsigned long startTime = millis();
    
    Serial.println();
    Serial.println("Received header:");
    
    for (int i = 0; i < 4; i++) {
      Serial.print(packetHead[i], HEX);
      Serial.print(",");
    }
    Serial.println();
    int len = packetHead[2] + packetHead[3]*256; //Does not include the checksum or the header
    
    if(packetHead[1] == 0x68) {//Ready check - only required in the initial handshake
      //Serial.println("Packet was ready check.");
      hasReceivedHandshake = true;
      par_put(ACK, 4);
      //Serial.println("Sent ACK.");
    } else if (hasReceivedHandshake && packetHead[1] == 0x15) { //Data - used for everything except the handshake
      if (len != 0) {
        par_get_wait(packetData, len);
        
        Serial.println("Received data:");
        
        for (int i = 0; i < len; i++) {
          Serial.print(packetData[i], HEX);
          Serial.print(",");
        }
        Serial.println();
        par_get_wait(packetChecksum, 2);
        int chksm = packetChecksum[0] + packetChecksum[1]*256;
        //Serial.println("Received checksum:");
        //Serial.println(chksm, HEX);
        int calcChksm = checksum(packetData, len);
        if (calcChksm == chksm) {
          //Serial.println("Checksum is valid.");
          par_put(ACK, 4);
          Serial.println("Sent ACK.");
          //Serial.print("Time elapsed (ms): ");
          //Serial.println(millis() - startTime);
          //Stop waiting and return
          keepWaiting = false;
        } else {
          //Serial.print("ERROR: Expected checksum: ");
          //Serial.println(calcChksm, HEX);
          par_put(ERR, 4);
          //Serial.println("Requested retransmission.");
        }
      }
    } else {
      //Either we haven't received the handshake yet or the command fell on deaf ears; reject the packet.
      if (len) {
        par_get_wait(packetData, len);
        par_get_wait(packetChecksum, 2);
      }
      par_put(NAK, 7);
      //Serial.println("Sent NAK.");
    }
  }
}

/**
 * Waits to receive the specified length of data into the pointer.
 * Retries until the receive doesn't time out.
 * Also implements autoshutdown and clearing of the protection disabled flag
 * since this is where the program spends a lot of its time waiting.
 */

int par_get_wait(byte *data, unsigned long len) {
  bool sentWarning = false;
  while (par_get(data, len)) {
    if (millis() - lastCmdReceived > 1000) protectionDisabled = false; //Clear the no-op flag after 1 second
    if (!sentWarning && ((millis() - lastCmdReceived) > autoShutDown)) { //If the time since the last command is more than the auto shut down time
      byte statusPacket[] = {MACHINE_ID, 0x15, 1, 0, 0xFE, 0xFE, 0};
      par_put(statusPacket, 7);
      free(&statusPacket);
      sentWarning = true;
      //Serial.println("Sent inactivity warning");
    } else if (sentWarning && ((millis() - lastCmdReceived) > (autoShutDown - 60000))) { //After the first warning, wait 60 seconds and then get ready to power down
      protectionDisabled = true; //Must disable protection in order to shut down
      //Serial.println("Shutting down...");
      initShutDown();
    }
  }
  lastCmdReceived = millis();
}

/**
 * Sends data to the calculator using the same command as the last one received.
 */

void reply(byte *data, unsigned long len) {
  packetHead[0] = MACHINE_ID;
  packetHead[1] = 0x15;
  packetHead[2] = len & 0xFF;
  packetHead[3] = len >> 8;
  packetData[0] = cmd;
  for (int i = 0; i < len; i++) {
    packetData[i + 1] = data[i];
  }
  word chksm = checksum(packetData, len + 1);
  packetChecksum[0] = chksm & 0xFF;
  packetChecksum[1] = chksm >> 8;
  par_put(packetHead, 4);
  par_put(packetData, len + 1);
  par_put(packetChecksum, 2);
/*
  byte returnPacket[len + 7];
  returnPacket[0] = MACHINE_ID;
  returnPacket[1] = 0x15;
  returnPacket[2] = len & 0xFF;
  returnPacket[3] = len >> 8;
  returnPacket[4] = cmd;
  for (int i = 0; i < len; i++) {
    returnPacket[i + 5] = data[i];
  }
  word chksm = checksum(returnPacket + 4, len + 1);
  returnPacket[len + 5] = chksm & 0xFF;
  returnPacket[len + 6] = chksm >> 8;
  par_put(returnPacket, len + 7);*/
}

/**
 * Arduino to TI linking routines by Christopher "Kerm Martian" Mitchell
 * http://www.cemetech.net/forum/viewtopic.php?t=4771
 */

void resetLines() {
   pinMode(TIring, INPUT);           // set pin to input
   digitalWrite(TIring, HIGH);       // turn on pullup resistors
   pinMode(TItip, INPUT);            // set pin to input
   digitalWrite(TItip, HIGH);        // turn on pullup resistors
}
static int par_put(uint8_t *data, uint32_t len) {
   int bit;
   int i, j;
   long previousMillis = 0;
   uint8_t byte;

   for(j=0;j<len;j++) {
      byte = data[j];
      for (bit = 0; bit < 8; bit++) {
         previousMillis = 0;
         while ((digitalRead(TIring)<<1 | digitalRead(TItip)) != 0x03) {
            if (previousMillis++ > TIMEOUT)
               return ERR_WRITE_TIMEOUT+j+100*bit;
         };
         if (byte & 1) {
            pinMode(TIring,OUTPUT);
            digitalWrite(TIring,LOW);
            previousMillis = 0;
            while (digitalRead(TItip) == HIGH) {
               if (previousMillis++ > TIMEOUT)
                  return ERR_WRITE_TIMEOUT+10+j+100*bit;
            };

            resetLines();
            previousMillis = 0;
            while (digitalRead(TItip) == LOW) {
               if (previousMillis++ > TIMEOUT)
                  return ERR_WRITE_TIMEOUT+20+j+100*bit;
            };
         } else {
            pinMode(TItip,OUTPUT);
            digitalWrite(TItip,LOW);      //should already be set because of the pullup resistor register
            previousMillis = 0;
            while (digitalRead(TIring) == HIGH) {
               if (previousMillis++ > TIMEOUT)
                  return ERR_WRITE_TIMEOUT+30+j+100*bit;
            };

            resetLines();
            previousMillis = 0;
            while (digitalRead(TIring) == LOW) {
               if (previousMillis++ > TIMEOUT)
                  return ERR_WRITE_TIMEOUT+40+j+100*bit;
            };
         }
         byte >>= 1;
      }
      //delayMicroseconds(6);
   }
   return 0;
}

static int par_get(uint8_t *data, uint32_t len) {
   int bit;
   int i, j;
   long previousMillis = 0;

   for(j = 0; j < len; j++) {
      uint8_t v, byteout = 0;
      for (bit = 0; bit < 8; bit++) {
         previousMillis = 0;
         while ((v = (digitalRead(TIring)<<1 | digitalRead(TItip))) == 0x03) {
            if (previousMillis++ > GET_ENTER_TIMEOUT)
               return ERR_READ_TIMEOUT+j+100*bit;
         }
         if (v == 0x01) {
            byteout = (byteout >> 1) | 0x80;
            pinMode(TItip,OUTPUT);
            digitalWrite(TItip,LOW);      //should already be set because of the pullup resistor register
            previousMillis = 0;
            while (digitalRead(TIring) == LOW) {            //wait for the other one to go low
               if (previousMillis++ > TIMEOUT)
                  return ERR_READ_TIMEOUT+10+j+100*bit;
            }
            //pinMode(TIring,OUTPUT);
            digitalWrite(TIring,HIGH);
         } else {
            byteout = (byteout >> 1) & 0x7F;
            pinMode(TIring,OUTPUT);
            digitalWrite(TIring,LOW);      //should already be set because of the pullup resistor register
            previousMillis = 0;
            while (digitalRead(TItip) == LOW) {
               if (previousMillis++ > TIMEOUT)
                  return ERR_READ_TIMEOUT+20+j+100*bit;
            }
            //pinMode(TItip,OUTPUT);
            digitalWrite(TItip,HIGH);
         }
         pinMode(TIring, INPUT);           // set pin to input
         digitalWrite(TIring, HIGH);       // turn on pullup resistors
         pinMode(TItip, INPUT);            // set pin to input
         digitalWrite(TItip, HIGH);        // turn on pullup resistors
      }
      data[j] = byteout;
      //delayMicroseconds(6);
   }
   return 0;
}

/**
 * Modified checksum calculation from the TI link guide
 * http://merthsoft.com/linkguide/ti83+/packet.html
 */
word checksum(byte *data, word datalength) {
  word checksum;
  for(int x = 0; x < datalength; x++) {
    checksum += data[x];  //overflow automatically limits to 16 bits
  }
  return checksum;
}