/*
    Arduino Love Button Library

    Copyright (c) 2023 Arduino SA

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

/*
The example works also without installing any capacitor on the Pin 
To obtain a better signal just install a capacitor 
-PIN 10 of Arduino UNO-R4 Minima 
-PIN 7  of Arduino UNO-R4 Wifi

If you touch the heart shaped pad on the bottom of the PCB it will behave as a capacitive touch button

You can interact with the library in 2 ways 
-read_touch() : You can set a threshold using setThreshold(threshold) 
                If the value read by the touch sensor exceed the threshold value, the read_touch() funtion
                will toogle it's returned value.
                Comparing new value with old value will let you understand if a Touch Event happened
-read_value() : It will return the raw value from the Capacitive sensor

*/

#include "Arduino_LoveButton.h"

bool      oldValue = false;
uint8_t   ledState = LOW;
uint16_t  sensValue;

void setup() {
  Serial.begin(115200);
  // Built-In LED 
  pinMode(13, OUTPUT);
  Arduino_Love.begin();  
  Arduino_Love.setThreshold(10000);
}

void loop() {
  //Check if a Touch event happened
  bool newValue = Arduino_Love.read_touch();

  //Read the Raw Data from the sensor
  sensValue = Arduino_Love.read_value();
  Serial.print("Read RAW Value from CTSU peripheral ");
  Serial.println(sensValue);

  //Compare old value with new one to undestand if a Touch event Happened
  if (newValue && !oldValue) {
    //Toogle the LED state in case of a Touch Event
    ledState = 1 - ledState;
    digitalWrite(13, ledState);
  }
  oldValue = newValue;  // save the state for next time
  delay(50);         // for debounce a little
}
