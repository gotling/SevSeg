/*
This library allows an Arduino to easily display numbers and characters on a 4 digit 7-segment
display without a separate 7-segment display controller.

If you have feature suggestions or need support please use the github support page: https://github.com/sparkfun/SevSeg

Original Library by Dean Reading (deanreading@hotmail.com: http://arduino.cc/playground/Main/SevenSegmentLibrary), 2012
Improvements by Nathan Seidle, 2012

Now works for any digital pin arrangement, common anode and common cathode displays.
Added character support including letters A-F and many symbols.

Hardware Setup: 4 digit 7 segment displays use 12 digital pins. You may need more pins if your display has colons or
apostrophes.

There are 4 digit pins and 8 segment pins. Digit pins are connected to the cathodes for common cathode displays, or anodes
for common anode displays. 8 pins control the individual segments (seven segments plus the decimal point).

Connect the four digit pins with four limiting resistors in series to any digital or analog pins. Connect the eight segment
pins to any digital or analog pins (no limiting resistors needed). See the SevSeg example for more connection information.

SparkFun has a large, 1" 7-segment display that has four digits.
https://www.sparkfun.com/products/11408
Looking at the display like this: 8.8.8.8. pin 1 is on the lower row, starting from the left.
Pin 12 is the top row, upper left pin.

Pinout:
1: Segment E
2: Segment D
3: Segment DP
4: Segment C
5: Segment G
6: Digit 4
7: Segment B
8: Digit 3
9: Digit 2
10: Segment F
11: Segment A
12: Digit 1


Software:
Call SevSeg.Begin in setup.
The first argument (boolean) tells whether the display is common cathode (0) or common
anode (1).
The next four arguments (bytes) tell the library which arduino pins are connected to
the digit pins of the seven segment display.  Put them in order from left to right.
The next eight arguments (bytes) tell the library which arduino pins are connected to
the segment pins of the seven segment display.  Put them in order a to g then the dp.

In summary, Begin(type, digit pins 1-4, segment pins a-g, dp)

The calling program must run the DisplayString() function repeatedly to get the number displayed.
Any number between -999 and 9999 can be displayed.
To move the decimal place one digit to the left, use '1' as the second
argument. For example, if you wanted to display '3.141' you would call
myDisplay.DisplayString("3141", 1);


*/

#include "SevSeg.h"

SevSeg::SevSeg() {
}

void SevSeg::Begin(boolean mode_in, byte numOfDigits,
	byte digit1, byte digit2, byte digit3, byte digit4,
	byte latch, byte data, byte clock) {
	numberOfDigits = numOfDigits;

	DigitPins[0] = digit1;
	DigitPins[1] = digit2;
	DigitPins[2] = digit3;
	DigitPins[3] = digit4;

	// We could skip SegmentPins array all together
	for (int i = 0; i < 8; ++i) {
		SegmentPins[i] = i;
	}

	latchPin = latch;
	dataPin = data;
	clockPin = clock;
	
	//Assign input values to variables
	//mode is what the digit pins must be set at for it to be turned on. 0 for common cathode, 1 for common anode
	mode = mode_in;
	if(mode == COMMON_ANODE) {
		DigitOn = HIGH;
		DigitOff = LOW;
		SegOn = LOW;
		SegOff = HIGH;
	} else {
		DigitOn = LOW;
		DigitOff = HIGH;
		SegOn = HIGH;
		SegOff = LOW;
	}
	
	//Turn everything Off before setting pin as output
	//Set all digit pins off. Low for common anode, high for common cathode
	for (byte digit = 0 ; digit < numberOfDigits ; digit++) {
		digitalWrite(DigitPins[digit], DigitOff);
		pinMode(DigitPins[digit], OUTPUT);
	}

	pinMode(latchPin, OUTPUT);
	pinMode(dataPin, OUTPUT);
	pinMode(clockPin, OUTPUT);

	//clear everything out just in case to
	//prepare shift register for bit shifting
	digitalWrite(dataPin, LOW);
	digitalWrite(clockPin, LOW);

	if (SegOff) {
		shiftWrite(0x00);
	} else {
		shiftWrite(0xFF);
	}
}

//Set the display brightness
/*******************************************************************************************/
//Given a value between 0 and 100 (0% and 100%), set the brightness variable on the display
//We need to error check and map the incoming value
void SevSeg::SetBrightness(byte percentBright) {
	//Error check and scale brightnessLevel
	if(percentBright > 100) {
		percentBright = 100;
	}
	brightnessDelay = map(percentBright, 0, 100, 0, FRAMEPERIOD); //map brightnessDelay to 0 to the max which is framePeriod
}


void SevSeg::shiftOut(byte myDataOut) {
	// This shifts 8 bits out MSB first, 
	//on the rising edge of the clock,
	//clock idles low

	//internal function setup
	int pinState;

	//for each bit in the byte myDataOut�
	//NOTICE THAT WE ARE COUNTING DOWN in our for loop
	//This means that %00000001 or "1" will go through such
	//that it will be pin Q0 that lights. 
	for (int i=7; i>=0; i--)  {
		digitalWrite(clockPin, 0);

		//if the value passed to myDataOut and a bitmask result 
		// true then... so if we are at i=6 and our value is
		// %11010100 it would the code compares it to %01000000 
		// and proceeds to set pinState to 1.
		if ( myDataOut & (1<<i) ) {
			pinState= 1;
		}
		else {  
			pinState= 0;
		}

		//Sets the pin to HIGH or LOW depending on pinState
		digitalWrite(dataPin, pinState);
		//register shifts bits on upstroke of clock pin  
		digitalWrite(clockPin, 1);
		//zero the data pin after shift to prevent bleed through
		digitalWrite(dataPin, 0);
	}

	//stop shifting
	digitalWrite(clockPin, 0);
}

void SevSeg::shiftWrite(byte data) {
	digitalWrite(latchPin, LOW);

	shiftOut(data);

	digitalWrite(latchPin, HIGH);
}

byte flipByte(byte c) {
  char r=0;
  for(byte i = 0; i < 8; i++){
    r <<= 1;
    r |= c & 1;
    c >>= 1;
  }
  return r;
}

//Refresh Display
/*******************************************************************************************/
//Given a string such as "-A32", we display -A32
//Each digit is displayed for ~2000us, and cycles through the 4 digits
//After running through the 4 numbers, the display is turned off
//Will turn the display on for a given amount of time - this helps control brightness
void SevSeg::DisplayString(char* toDisplay, byte DecAposColon) {
	//For the purpose of this code, digit = 1 is the left most digit, digit = 4 is the right most digit
	for(byte digit = 0 ; digit < numberOfDigits; digit++) {
		digitalWrite(DigitPins[digit], DigitOn);
		
		//Here we access the array of segments
		//This could be cleaned up a bit but it works
		//displayCharacter(toDisplay[digit-1]); //Now display this digit
		// displayArray (defined in SevSeg.h) decides which segments are turned on for each number or symbol
		char characterToDisplay = toDisplay[digit];

		byte character = pgm_read_byte(&characterArray[characterToDisplay]);
		
		shiftWrite(~characterToDisplay);
		//shiftWrite(0xCC);

		//Service the decimal point, apostrophe and colon
		// if ((DecAposColon & (1<<(digit))) && (digit < 5)) //Test DecAposColon to see if we need to turn on a decimal point
		// 	digitalWrite(segmentDP, SegOn);
		
		delayMicroseconds(brightnessDelay + 1); //Display this digit for a fraction of a second (between 1us and 5000us, 500-2000 is pretty good)
		//The + 1 is a bit of a hack but it removes the possible zero display (0 causes display to become bright and flickery)
		//If you set this too long, the display will start to flicker. Set it to 25000 for some fun.
		
		//Turn off all segments
		shiftWrite(0xFF);
		
		//Turn off this digit
		digitalWrite(DigitPins[digit], DigitOff);

		// The display is on for microSeconds(brightnessLevel + 1), now turn off for the remainder of the framePeriod
		delayMicroseconds(FRAMEPERIOD - brightnessDelay + 1); //the +1 is a hack so that we can never have a delayMicroseconds(0), causes display to flicker
	}
}
