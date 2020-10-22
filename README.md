# Basic IMU Joystick
Game controller, based on the MPU6050 gyroscope & Arduino Pro Micro.
See https://github.com/SimonMaier/MarioKartHelmet form more infos.

For the Giant Joystick use the Branch `GiantJoystick` : https://github.com/HackerLoop/BasicHeadTracker/tree/GiantJoystick

## Usage
Plug the USB directly on the computer.

### Test
Go to https://gamepad-tester.com/

## Build

MPU6050

VCC -> VCC
GND -> GND
SCL -> 2
SDA -> 3
AD0 -> GND
INT -> 7

pin A0 -> button a (onboard)
pin A1 -> LED onboard button

pin 15 -> TriggerL (connector 1 green)
pin 14 -> TriggerR (connector 1 blue)
pin 16 -> button B (connector 2 green)
pin 10 -> button START (connector 2 blue)

pin RST -> Button RESET

### Important
- Keep wire short Between MPU and Arduino (may cause issue if wire are to long)

### Arduino IDE

1. Install external Arduino libraries:
  - [i2cdevlib](https://github.com/jrowberg/i2cdevlib) collection (at least [I2Cdev](https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/I2Cdev) and [MPU6050](https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/MPU6050)).

    Follow the [installation instructions on the website](https://www.i2cdevlib.com/usage), or copy the folders `i2cdevlib/Arduino/I2Cdev` and `i2cdevlib/Arduino/MPU6050` from the project repository into your Arduino [libraries directory](https://www.arduino.cc/en/hacking/libraries) (`~/Arduino/libraries/` on Linux).

  - [ArduinoXInput_AVR](https://github.com/dmadison/ArduinoXInput_AVR)

2. Upload the software (select Arduino Leonardo w/Xinput as a target board)

  !!!!!!! IMPORTANT !!!!!!!
  Due to the nature of how the XInput USB mode works, Arduinos that have XInput sketches on them will not automatically reset when programmed by the IDE!

  You will need to press the reset button twice quickly every time you upload new code.

  How to Upload: https://github.com/dmadison/ArduinoXInput_AVR