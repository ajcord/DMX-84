void waitForPacket();
void resetLines();
int par_get_wait(byte *data, unsigned long len);
static int par_put(byte *data, unsigned long len);
static int par_get(byte *data, unsigned long len);
word calculateChecksum(byte *data, word datalength);

const unsigned long autoShutDown = 21600000; //Number of milliseconds before shutting down (6 hours)
#define MACHINE_ID 0x23
#define LED_PIN 9

byte ACK[] = {MACHINE_ID, 0x56, 0, 0}; //Acknowledge
byte NAK[] = {MACHINE_ID, 0x36, 1, 0, 1, 1, 0}; //Skip/Exit
byte CTS[] = {MACHINE_ID, 0x09, 0, 0}; //Clear to send
byte ERR[] = {MACHINE_ID, 0x5A, 0, 0}; //Checksum error

int len = 0;
byte packetHead[4] = {0};
byte packetData[1024] = {0};
byte receivedChecksum[2] = {0};
bool hasReceivedHandshake = false;


bool wasLastCmdNoOp = false; //Stores whether the last command was a no-op
unsigned long lastCmdReceived; //Stores the time the last command was received

void setup() {
  //Set up the I/O lines
  resetLines();
  //Not sure what I'll end up using the LED for. For now, it will be a power indicator.
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Serial.begin(115200);
  Serial.setTimeout(1000);
  Serial.println("Ready");
}

void loop() {
  Serial.println("Beginning to wait for packet...");
  waitForPacket();
  Serial.println("Done waiting for packet.");
}

void waitForPacket() {
  //Waits for a valid packet to be received and returns the array of data.
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
    len = packetHead[2] + packetHead[3]*256; //Does not include the checksum or the header
    
    if(packetHead[1] == 0x68) {//Ready check - only required in the handshake
      Serial.println("Packet was ready check.");
      hasReceivedHandshake = true;
      par_put(ACK, 4);
      Serial.println("Sent ACK.");
    } else if (hasReceivedHandshake && packetHead[1] == 0x15) { //Data - used for everything except the handshake
      if (len != 0) {
        par_get_wait(packetData, len);
        Serial.println("Received data:");
        for (int i = 0; i < len; i++) {
          Serial.print(packetData[i], HEX);
          Serial.print(",");
        }
        Serial.println();
        par_get_wait(receivedChecksum, 2);
        int chksm = receivedChecksum[0] + receivedChecksum[1]*256;
        Serial.println("Received checksum:");
        Serial.println(chksm, HEX);
        int calcChksm = checksum(packetData, len);
        if (calcChksm == chksm) {
          Serial.println("Checksum is valid.");
          par_put(ACK, 4);
          Serial.println("Sent ACK.");
          Serial.print("Time elapsed (ms): ");
          Serial.println(millis() - startTime);
          //Stop waiting and return
          keepWaiting = false;
        } else {
          Serial.print("ERROR: Expected checksum: ");
          Serial.println(calcChksm, HEX);
          par_put(ERR, 4);
          Serial.println("Requested retransmission.");
        }
      }
    } else {
      //Either we haven't received the handshake yet or the command byte fell on deaf ears; reject the packet.
      if (len) 
        par_get_wait(packetData, len);
        par_get_wait(receivedChecksum, 2);
      par_put(NAK, 7);
      Serial.println("Sent NAK.");
    }
  }
}

void initShutDown() {
  byte statusPacket[] = {MACHINE_ID, 0x15, 1, 0, 0xFF, 0xFF, 0};
  par_put(statusPacket, 7);
  byte EOT[] = {MACHINE_ID, 0x92, 0, 0}; //End of transmission
  par_put(EOT, 4);
  Serial.println("System is going down NOW!");
  Serial.end(); //Stop serial communication
  digitalWrite(LED_PIN, LOW);
  while (true); //Go into an infinite loop
}

int par_get_wait(byte *data, unsigned long len) {
  bool sentWarning = false;
  while (par_get(data, len)) {
    if (millis() - lastCmdReceived > 1000) wasLastCmdNoOp = false; //Clear the no-op flag after 1 second
    if (!sentWarning && millis() - lastCmdReceived > autoShutDown) { //If the time since the last command is more than the auto shut down time
      byte statusPacket[] = {MACHINE_ID, 0x15, 1, 0, 0xFE, 0xFE, 0};
      par_put(statusPacket, 7);
      sentWarning = true;
      Serial.println("Sent inactivity warning");
    } else if (sentWarning && millis() - lastCmdReceived > autoShutDown + 5000) { //After the first warning, wait 60 seconds and then get ready to power down
      wasLastCmdNoOp = true;
      Serial.println("Shutting down...");
      initShutDown();
    }
  }
  lastCmdReceived = millis();
}

#define TIring 4
#define TItip 6
#define TIwhite TIring
#define TIred TItip
#define ERR_READ_TIMEOUT 1000
#define ERR_WRITE_TIMEOUT 2000
#define TIMEOUT 4000
#define GET_ENTER_TIMEOUT 30000

void resetLines() {
   pinMode(TIring, INPUT);           // set pin to input
   digitalWrite(TIring, HIGH);       // turn on pullup resistors
   pinMode(TItip, INPUT);            // set pin to input
   digitalWrite(TItip, HIGH);        // turn on pullup resistors
}
static int par_put(byte *data, unsigned long len) {
   int bit;
   int i, j;
   long previousMillis = 0;
   byte byte;

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

static int par_get(byte *data, unsigned long len) {
   int bit;
   int i, j;
   long previousMillis = 0;

   for(j = 0; j < len; j++) {
      byte v, byteout = 0;
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