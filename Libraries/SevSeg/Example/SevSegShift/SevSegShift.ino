#include "SevSeg.h"

const byte digitPin1 = 14;

const byte latchPin = 11;
const byte dataPin = 12;
const byte clockPin = 13;

const int displayType = COMMON_ANODE;
const int numberOfDigits = 4;

SevSeg sDisplay;

long timer;
int deciSecond = 0;

void setup() {
  sDisplay.Begin(displayType, numberOfDigits, digitPin1, digitPin1+1, digitPin1+2, digitPin1+3, latchPin, dataPin, clockPin);
  sDisplay.SetBrightness(100);
  
  timer = millis();
}

void loop() {
  char text[4];
  sprintf(text, "%4d", deciSecond);
  sDisplay.DisplayString(text, 0);
  
  if (millis() - timer >= 100) {
    timer = millis();
    deciSecond++;
  }
  
  delay(5);
}
