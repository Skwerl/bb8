/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* Libraries *////////////////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

#include <string.h>
#include <Arduino.h>
#include <Servo.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* Bluetooth Config */////////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

#define FACTORYRESET_ENABLE 0
#define MINIMUM_FIRMWARE_VERSION "0.6.6"
#define MODE_LED_BEHAVIOR "MODE"

SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);
Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN, BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);

uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t * data, const uint32_t numBytes);

extern uint8_t packetbuffer[];

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* A/B/C Motor Config *///////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

#define smcRxPinA 0
#define smcTxPinA 5

#define smcRxPinB 0
#define smcTxPinB 6

#define smcRxPinC 0
#define smcTxPinC 7

int mcenter = 0;
int mfactor = 320;
int mrange = 3200;

SoftwareSerial smcSerialA = SoftwareSerial(smcRxPinA, smcTxPinA);
SoftwareSerial smcSerialB = SoftwareSerial(smcRxPinB, smcTxPinB);
SoftwareSerial smcSerialC = SoftwareSerial(smcRxPinC, smcTxPinC);

void smcSafeStart() {
  smcSerialA.write(0x83);
  smcSerialB.write(0x83);
  smcSerialC.write(0x83);
}

void setMotorSpeed(SoftwareSerial &smcSerial, int speed) {
  if (speed < 0) {
    smcSerial.write(0x86);
    speed = -speed;
  } else {
    smcSerial.write(0x85);
  }
  smcSerial.write(speed & 0x1F);
  smcSerial.write(speed >> 5);
}

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* Sound Trigger Config */////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

#define mp3RxPin 2
#define mp3TxPin 3

int mp3Byte = 0;
boolean mp3Playing = false;

SoftwareSerial mp3Trigger = SoftwareSerial(mp3RxPin, mp3TxPin);

void playSound(int sample) {
  if (mp3Playing == false) {
    Serial.print("Playing sample #");
    Serial.println(sample);
    mp3Trigger.write('t');
    mp3Trigger.write(sample);
    // Need to get mp3Trigger.read() working...
    //mp3Playing = true;
  }
}

void stopSound() {
  if (mp3Playing == true) {
    Serial.println("Stop");
    mp3Trigger.write('O');
    mp3Playing = false;
  }
}

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* Neopixel Config *//////////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

#define NEO_A 4
Adafruit_NeoPixel strip_a = Adafruit_NeoPixel(3, NEO_A);

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* BB-8 Logic *///////////////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

/*
int ytarget = 0;
int xtarget = xcenter;

boolean driveN = false;
boolean driveS = false;
boolean driveE = false;
boolean driveW = false;

void restDrive(void) {
  ytarget = 0;
  xtarget = xcenter;
  driveN = false;
  driveS = false;
  driveE = false;
  driveW = false;
}
*/

void sendMove(void) {

  /*
  Serial.print("SEND: ");
  Serial.print(ytarget);
  Serial.print(",");
  Serial.print(xtarget);
  Serial.println();  
  */

  //xaxis.write(xtarget);
  //setMotorSpeed(smcSerialA, ytarget);

}

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* Arduino *//////////////////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

void setup(void) {

  // Lights!
  strip_a.begin();
  strip_a.setBrightness(80);
  strip_a.setPixelColor(0, 10, 30, 120);
  strip_a.setPixelColor(1, 40, 220, 250);
  strip_a.setPixelColor(2, 255, 255, 255);
  strip_a.show();

  // Serial, Motor
  smcSerialA.begin(19200);
  smcSerialB.begin(19200);
  smcSerialC.begin(19200);
  delay(5);
  smcSerialA.write(0xAA);
  smcSerialB.write(0xAA);
  smcSerialC.write(0xAA);
  smcSafeStart();

  // Serial, Sounds
  mp3Trigger.begin(38400);

  // Serial, Bluetooth / Monitor
  Serial.begin(115200);
  Serial.print(F("Initializing Bluefruit LE module: "));
  if (!ble.begin(VERBOSE_MODE)) {
    error(F("Couldn't find Bluefruit, make sure it's in CMD mode & check wiring?"));
  }
  Serial.println(F("OK!"));
  if (FACTORYRESET_ENABLE) {
    Serial.println(F("Performing a factory reset: "));
    if (!ble.factoryReset()) {
      error(F("Couldn't factory reset!"));
    }
  }
  ble.echo(false);
  Serial.println("Requesting Bluefruit info:");
  ble.info();
  /*
  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in Controller mode"));
  Serial.println(F("Then activate/use the sensors, color picker, game controller, etc!"));
  Serial.println();
  */
  ble.verbose(false);

  /* Wait for connection */
  while (!ble.isConnected()) {
      delay(500);
  }

  Serial.println(F("******************************"));

  if (ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION)) {
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOR);
  }

  Serial.println(F("Switching to DATA mode!"));
  ble.setMode(BLUEFRUIT_MODE_DATA);

  //Serial.println(F("******************************"));

}

void loop(void) {

  uint8_t len = readPacket(&ble, BLE_READPACKET_TIMEOUT);
  //if (len == 0) return;

  if (packetbuffer[1] == 'B') {

    uint8_t buttnum = packetbuffer[2] - '0';
    boolean pressed = packetbuffer[3] - '0';
  
    if (pressed) {

      switch(buttnum) {
        case 1:
          playSound(1);
          break;
        case 2:
          playSound(51);
          break;
        case 3:
          playSound(52);
          break;
        case 4:
          playSound(53);
          break;
        
        case 5: // up

          break;
        case 6: // down

          break;
        case 7:
          setMotorSpeed(smcSerialA, 1200);
          setMotorSpeed(smcSerialB, -1200);
          setMotorSpeed(smcSerialC, 1200);
          break;
        case 8:
          setMotorSpeed(smcSerialA, -1200);
          setMotorSpeed(smcSerialB, 1200);
          setMotorSpeed(smcSerialC, -1200);
          break;
      }
    
      Serial.print(buttnum);
      Serial.println("PRESSED");

    } else {

      switch(buttnum) {

         case 2:
          setMotorSpeed(smcSerialB, 0);
          break;       
        
        case 5:

          break;
        case 6:

          break;
        case 7:
        case 8:
          setMotorSpeed(smcSerialA, 0);
          setMotorSpeed(smcSerialB, 0);
          setMotorSpeed(smcSerialC, 0);
          break;
      }

      Serial.print(buttnum);
      Serial.println("RELEASED");

    }
  
  }

  /*
  if (driveN) { if (ytarget < (ycenter+yrange)) { ytarget += yfactor; } }
  if (driveS) { if (ytarget > (ycenter-yrange)) { ytarget -= yfactor; } }
  if (driveW) { if (xtarget < (xcenter+xrange)) { xtarget += xfactor; } }
  if (driveE) { if (xtarget > (xcenter-xrange)) { xtarget -= xfactor; } }

  if (!driveN && !driveS) {
     ytarget = 0;
  }

  if (!driveE && !driveW) {
     xtarget = xcenter;
  }
  */

  /*
  Serial.print(driveN); Serial.print(""); 
  Serial.print(driveS); Serial.print(""); 
  Serial.print(driveE); Serial.print(""); 
  Serial.print(driveW); Serial.print(""); 
  */

  /*
  Serial.print("YTARGET: ");
  Serial.print(ytarget);

  Serial.print(" // ");

  Serial.print("XTARGET: ");
  Serial.print(xtarget);

  Serial.println();
  */

  /*
  mp3Byte = mp3Trigger.read();
  Serial.print("I received: ");
  Serial.println(mp3Byte, DEC);
  if (mp3Byte == 88) {
    mp3Playing = false;
  }
  */

  sendMove();

}
