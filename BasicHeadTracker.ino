/*
 * Board: Arduino Leonardo
 * 
 * !!!!!!! IMPORTANT !!!!!!!
 * Due to the nature of how the XInput USB mode works, 
 * Arduinos that have XInput sketches on them will not 
 * automatically reset when programmed by the IDE! 
 * 
 * You will need to press the reset button twice quickly 
 * every time you upload new code.
 * 
 * How to Upload: https://github.com/dmadison/ArduinoXInput_AVR
*/
#include "I2Cdev.h"
#include "MPU6050_6Axis_MotionApps20.h"
#include "Wire.h"
#include <XInput.h>

MPU6050 mpu;

const int ADC_Max = 1023;  // 10 bit

// Trigger Pins
const int Pin_TriggerL = 15;
const int Pin_TriggerR = 14;

// Button Pins
const int Pin_ButtonB = 16;
const int Pin_ButtonStart = 10;

const int Pin_ButtonA = A0;

bool buttonPressed = false;
unsigned long onTime = 0;

// Led
const uint8_t LED_Pin = A1;
bool blinkState = false;

// MPU6050
#define SCALING 6        // max Joystick value  at 90deg/SCALING.

#define INTERRUPT_PIN 7  // use pin 2 on Arduino Uno & most boards

// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

float pitchOffset = 0;
float rollOffset = 0;

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() { mpuInterrupt = true; }


void setup() {
  pinMode(Pin_TriggerL, INPUT_PULLUP);
  pinMode(Pin_TriggerR, INPUT_PULLUP);
  pinMode(Pin_ButtonA, INPUT_PULLUP);
  pinMode(Pin_ButtonB, INPUT_PULLUP);
  
  pinMode(Pin_ButtonStart, INPUT_PULLUP);

  pinMode(LED_Pin, OUTPUT);    // Set LED pin as output
  digitalWrite(LED_Pin, HIGH);  // Turn LED off

  XInput.setReceiveCallback(rumbleCallback);

  XInput.setJoystickRange(-512, 511);  // Set joystick range to the ADC
  XInput.setAutoSend(false);  // Wait for all controls before sending

  
  XInput.begin();

  // set up MPU6050
  Wire.begin();
  Wire.setClock(400000);
  mpu.initialize();
  pinMode(INTERRUPT_PIN, INPUT);

  // load and configure the DMP
  devStatus = mpu.dmpInitialize();

  // make sure it worked (returns 0 if so)
  if (devStatus == 0) {
    // turn on the DMP, now that it's ready
    mpu.setDMPEnabled(true);

    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
    mpuIntStatus = mpu.getIntStatus();
    dmpReady = true;
    // get expected DMP packet size for later comparison
    packetSize = mpu.dmpGetFIFOPacketSize();
  } else {
    fail();
  }
}

void loop() {
  if (!dmpReady) return;

  boolean buttonA = !digitalRead(Pin_ButtonA);
  boolean buttonB = !digitalRead(Pin_ButtonB);
  boolean triggerLeft  = !digitalRead(Pin_TriggerL);
  boolean triggerRight = !digitalRead(Pin_TriggerR);

  boolean buttonStart = !digitalRead(Pin_ButtonStart);

  if (buttonA == HIGH && !buttonPressed) {
    buttonPressed = true;
    onTime = millis();
  } else if (buttonA == LOW && buttonPressed) {
    if ((millis() - onTime) > 1000 ) // half second hold time
    {
      pitchOffset = ypr[1];
      rollOffset = ypr[2];
    }
    buttonPressed = false;
  }
  
  // Set XInput buttons
  XInput.setButton(BUTTON_A, buttonA);
  XInput.setButton(BUTTON_B, buttonB);
  XInput.setButton(TRIGGER_LEFT, triggerLeft);
  XInput.setButton(TRIGGER_RIGHT, triggerRight);

  XInput.setButton(BUTTON_START, buttonStart);

  // wait for MPU interrupt or extra packet(s) available
  while (!mpuInterrupt && fifoCount < packetSize) {
    if (mpuInterrupt && fifoCount < packetSize) {
      // try to get out of the infinite loop 
      fifoCount = mpu.getFIFOCount();
    }
  }

  // reset interrupt flag and get INT_STATUS byte
  mpuInterrupt = false;
  mpuIntStatus = mpu.getIntStatus();

  // get current FIFO count
  fifoCount = mpu.getFIFOCount();

  // check for overflow (this should never happen unless our code is too inefficient)
  if ((mpuIntStatus & _BV(MPU6050_INTERRUPT_FIFO_OFLOW_BIT)) || fifoCount >= 1024) {
    // reset so we can continue cleanly
    mpu.resetFIFO();
    fifoCount = mpu.getFIFOCount();
    //Serial.println(F("FIFO overflow!"));

  // otherwise, check for DMP data ready interrupt (this should happen frequently)
  } else if (mpuIntStatus & _BV(MPU6050_INTERRUPT_DMP_INT_BIT)) {
    // wait for correct available data length, should be a VERY short wait
    while (fifoCount < packetSize) fifoCount = mpu.getFIFOCount();

    // read a packet from FIFO
    mpu.getFIFOBytes(fifoBuffer, packetSize);
    
    // track FIFO count here in case there is > 1 packet available
    // (this lets us immediately read more without waiting for an interrupt)
    fifoCount -= packetSize;

    // display Euler angles in degrees
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

    // Set left joystick
    int leftJoyX = (ypr[2]-rollOffset) * ADC_Max/M_PI*SCALING;
    int leftJoyY = (ypr[1]-pitchOffset) * ADC_Max/M_PI*SCALING;
    
    // White lie here... most generic joysticks are typically
    // inverted by default. If the "Invert" variable is false
    // then we need to do this transformation.
    leftJoyY = -leftJoyY;
    
    XInput.setJoystick(JOY_LEFT, leftJoyX, leftJoyY);

    // blink LED to indicate activity
    //blinkState = !blinkState;
    //digitalWrite(LED_PIN, blinkState);
  }

  // Send control data to the computer
  XInput.send();
}

void rumbleCallback(uint8_t packetType) {
  // If we have an LED packet (0x01), do nothing
  if (packetType == (uint8_t) XInputReceiveType::LEDs) {
    //return;
  }

  // If we have a rumble packet (0x00), see our rumble data on the LED
  else if (packetType == (uint8_t) XInputReceiveType::Rumble) {
    int rumbleValue = XInput.getRumbleLeft() | XInput.getRumbleRight();
    digitalWrite(LED_Pin, !rumbleValue);
  }
}

void fail() {
  while (1) {
    RXLED0;
    delay(250);
    RXLED1;
    delay(250);
  }
}
