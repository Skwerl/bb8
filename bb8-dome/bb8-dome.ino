/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* Libraries *////////////////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

#include <Servo.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

#include "SwitchBT.h"
#include <usbhub.h>

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* Bluetooth Config */////////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

USB Usb;
BTD Btd(&Usb);

// To Pair:
//SwitchBT Switch(&Btd, PAIR);

// After Pair:
SwitchBT Switch(&Btd);

bool bluetoothInit = false;

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* A/B/C Motor Config *///////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

// Blue
#define smcRxPinA 0
#define smcTxPinA 5

// Green
#define smcRxPinB 0
#define smcTxPinB 4

// Purple
#define smcRxPinC 0
#define smcTxPinC 6

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

void moveTowards(float degree, int magnitude) {
  
  float theta = degree/57.2957795; 
  
  float vx = cos(theta)*magnitude;
  float vy = sin(theta)*magnitude;
  float sqrt3o2 = 1.0*sqrt(3)/2;
  
  float w1 = -vx;
  float w2 = 0.5*vx - sqrt3o2 * vy;
  float w3 = 0.5*vx + sqrt3o2 * vy;
  
  setMotorSpeed(smcSerialA, w1);
  setMotorSpeed(smcSerialB, w2);
  setMotorSpeed(smcSerialC, w3);

}

void stopAllMotors() {
  setMotorSpeed(smcSerialA, 0);
  setMotorSpeed(smcSerialB, 0);
  setMotorSpeed(smcSerialC, 0);
}

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* Sound Trigger Config */////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

#define mp3RxPin 2
#define mp3TxPin 3

int volume = 255;
int mp3Byte = 0;
bool mp3Playing = false;

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

//#define NEO_A 4
//Adafruit_NeoPixel strip_a = Adafruit_NeoPixel(2, NEO_A);

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* Switch Parsing *///////////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

byte* Report;

struct SwitchButtons {
  bool Idle;
  bool U_Button;
  bool D_Button;
  bool L_Button;
  bool R_Button;
  bool Y_Button;
  bool X_Button;
  bool B_Button;
  bool A_Button;
  bool SL_Button;
  bool SR_Button;
  bool L_Trigger;
  bool ZL_Trigger;
  bool R_Trigger;
  bool ZR_Trigger;
  bool Stick_Button;
  bool Select_Button;
  bool Action_Button;
  int Analog_Stick;
};
SwitchButtons reset = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
SwitchButtons state = reset;

void handleEvent() {
  
  if (Report[0] == 0 && Report[1] == 0 && Report[2] == 8) {

    allButtonsReleased();
    state.Idle = 1;

  } else {

    state.Idle = 0;

    switch(Report[0]) {
      case 4:
        state.U_Button = 1;
        state.B_Button = 1;
        break;
      case 2:
        state.D_Button = 1;
        state.X_Button = 1;
        break;
      case 1:
        state.L_Button = 1;
        state.A_Button = 1;
        break;
      case 8:
        state.R_Button = 1;
        state.Y_Button = 1;
        break;
      case 16:
        state.SL_Button = 1;
        break;
      case 32:
        state.SR_Button = 1;
        break;
    }
    switch(Report[1]) {
      case 64:
        state.L_Trigger = 1;
        state.R_Trigger = 1;
        break;
      case 128:
        state.ZL_Trigger = 1;
        state.ZR_Trigger = 1;
        break;
      case 1:
      case 2:
        state.Select_Button = 1;
        break;
      case 32:
        state.Action_Button = 1;
        break;
      case 4:
      case 8:
        state.Stick_Button = 1;
        break;
    }
    switch(Report[2]) {
      case 8:
        state.Analog_Stick = 0;
        break;
      case 7:
        state.Analog_Stick = 45;
        break;
      case 0:
        state.Analog_Stick = 90;
        break;
      case 1:
        state.Analog_Stick = 135;
        break;
      case 2:
        state.Analog_Stick = 180;
        break;
      case 3:
        state.Analog_Stick = 225;
        break;
      case 4:
        state.Analog_Stick = 270;
        break;
      case 5:
        state.Analog_Stick = 315;
        break;
      case 6:
        state.Analog_Stick = 360;
        break;
    }

  }

  if (!bluetoothInit) {
    if (state.Analog_Stick > 0) {
      return;    
    } else {
      bluetoothInit = true;
      Serial.println("");
      Serial.println("Bluetooth Connection Established");
      wakeUp();
    }
  } else {

    /*
    Serial.print(state.Idle);
    Serial.print(state.Y_Button);
    Serial.print(state.X_Button);
    Serial.print(state.B_Button);
    Serial.print(state.A_Button);
    Serial.print(state.SL_Button);
    Serial.print(state.SR_Button);
    Serial.print(state.R_Trigger);
    Serial.print(state.ZR_Trigger);
    Serial.print(state.Stick_Button);
    Serial.print(state.Select_Button);
    Serial.print(state.Action_Button);
    Serial.print(state.Analog_Stick);
    Serial.println("");
    */
    
    doAction();
    
  }

}

void allButtonsReleased() {
  state = reset;
}

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* BB-8 Logic *///////////////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

#define projectorPin 19
bool projectorOn = false;

void toggleProjector() {
  if (!already_done()) {
    if (projectorOn == true) {
      digitalWrite(projectorPin, LOW);
      projectorOn = false;
    } else {
      digitalWrite(projectorPin, HIGH);
      projectorOn = true;
    }
    now_done();
  }
}

void doAction() {

  if (state.B_Button) {
    playSound(1);
  }

  if (state.R_Trigger) {
    toggleProjector();
  }

  if (state.ZR_Trigger && state.Analog_Stick == 90) {
    setMotorSpeed(smcSerialA, 1000);
    setMotorSpeed(smcSerialB, 1000);
    setMotorSpeed(smcSerialC, 1000);
  }

  if (state.ZR_Trigger && state.Analog_Stick == 270) {
    setMotorSpeed(smcSerialA, -1000);
    setMotorSpeed(smcSerialB, -1000);
    setMotorSpeed(smcSerialC, -1000);
  }

  if (state.Analog_Stick <= 0) {
    stopAllMotors();
  }

}

void wakeUp() {
  playSound(21);
}

/*////////////////////////////////////////////////////////////////////////////////////////////////*/
///////////////////////* Timing Control *///////////////////////////////////////////////////////////
/*////////////////////////////////////////////////////////////////////////////////////////////////*/

#define trigger_gate 4096
int trigger_counter = trigger_gate;

void increment_counter(void) {
  if (trigger_counter > 32000) { trigger_counter = trigger_gate; } trigger_counter++;
}

bool already_done() {
  if (trigger_counter <= trigger_gate) {
    return true;
  }
  return false;
}

void now_done() {
  trigger_counter = 0;
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
  pinMode(projectorPin, OUTPUT);
  digitalWrite(projectorPin, LOW);
  /*
  strip_a.begin();
  strip_a.setBrightness(255);
  strip_a.setPixelColor(0, 0, 0, 20);
  strip_a.setPixelColor(1, 0, 70, 30);
  strip_a.show();
  */
  
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
  
  byte volbyte = map((volume-255), 0, 1023, 0, 255);
  mp3Trigger.write('v');
  mp3Trigger.write(volbyte);

  // Bluetooth
  Serial.begin(115200);
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while (1); // Halt
  }
  Serial.print(F("\r\nSwitch Bluetooth Library Started"));

}

void loop(void) {

  Usb.Task();

  if (Switch.connected()) {

    Report = Switch.Report;
    handleEvent();
    
  }

  increment_counter();

}
