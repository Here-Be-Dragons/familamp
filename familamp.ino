#include "application.h"
#include "captouch.h"
#include "neopixel/neopixel.h"

// Define which pins are connected with a 1-10M resistor.
// The first pin will be connected to the touch sensor
// and must be D0, D1, D2, D3, D4 A0, A1, A3, A4, A5, A6, A7
// see: http://docs.spark.io/firmware/#interrupts-attachinterrupt
CapTouch Touch(D3, D4);

#define PIXEL_COUNT 16
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

double lastColorUpdate = 0;  //Tracks when we last got sent a color
uint16_t colorFromID;       //Tracks who sent the color
uint32_t colorRecieved;     //Tracks the color recieved from another lamp
uint16_t activeColor;       //Tracks what color is currently active
uint16_t decayTime = 300;         //Turn off light after elapsed milliseconds
CapTouch::Event touchEvent;

// Time Zone offset
int16_t timeZone = -5;

void setup() {
    Touch.setup();
    //Listen for other lamps to send a particle.publish()
    Particle.subscribe("Familamp", gotColorUpdate, MY_DEVICES);
    strip.begin();
    for (int i = 0; i < strip.numPixels(); i++){
        strip.setPixelColor(i, 255, 255, 255);
    }
    strip.show();
    delay(10);
	strip.show(); // Initialize all pixels to 'off'
    for (int i = 0; i < strip.numPixels(); i++){
        strip.setPixelColor(i, 0, 0, 0);
    }
	strip.show(); // Initialize all pixels to 'off'
}

void loop() {
    touchEvent = Touch.getEvent();

    if (touchEvent == CapTouch::TouchEvent) {
		whileTouching();
		sendColorUpdate(activeColor);
	}
    if (Time.now() - lastColorUpdate > decayTime) {
        setColor(0);
    }
    delay(100);
}

void whileTouching() {
    uint32_t i = 0;
    while (touchEvent != CapTouch::ReleaseEvent) {
        //uint16_t color = wheelColor(i,255);
        uint16_t color = wheelColor(i);
        activeColor = setColor(color);
        i++;
        i %= 255;
        delay(1);
        touchEvent = Touch.getEvent();
    }
}

void sendColorUpdate(uint16_t c) {
    char publishString[40];
    sprintf(publishString, "%u", c);
    Particle.publish("updateFamilamp", publishString);
    lastColorUpdate = Time.now();
}

void gotColorUpdate(const char *name, const char *data) {
    
    String str = String(data);
    char strBuffer[400] = "";
    str.toCharArray(strBuffer, 400);
    lastColorUpdate = Time.now();
    
    colorFromID = atof(strtok(strBuffer, "~"));
    colorRecieved = atof(strtok(NULL, "~"));
    activeColor = setColor(colorRecieved);
}

uint16_t setColor(uint16_t c) {
    for (byte j = 0; j < strip.numPixels(); j++) {
		strip.setPixelColor(j, c);
    }
    strip.show();
    return c;
}

uint32_t wheelColor(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
/*
uint16_t wheelColor(byte WheelPos, byte iBrightness) {
	float R, G, B;
	float brightness = iBrightness / 255.0;

	if (WheelPos < 85) {
		R = WheelPos * 3;
		G = 255 - WheelPos * 3;
		B = 0;
	} else if (WheelPos < 170) {
		WheelPos -= 85;
		R = 255 - WheelPos * 3;
		G = 0;
		B = WheelPos * 3;
	} else {
		WheelPos -= 170;
		R = 0;
		G = WheelPos * 3;
		B = 255 - WheelPos * 3;
	}
	R = R * brightness + .5;
	G = G * brightness + .5;
	B = B * brightness + .5;
	return strip.Color((byte) R,(byte) G,(byte) B);
}*/
