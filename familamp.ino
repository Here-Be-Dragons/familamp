#include "application.h"
#include "captouch.h"
#include "neopixel/neopixel.h"

SYSTEM_MODE(AUTOMATIC);

//PRODUCT_ID and PRODUCT_VERSION required for Particle's "Productions" feature
PRODUCT_ID(639);
PRODUCT_VERSION(3);

// Define which pins are connected with a 1-10M resistor.
// The first pin will be connected to the touch sensor
// and must be D0, D1, D2, D3, D4 A0, A1, A3, A4, A5, A6, A7
// see: http://docs.spark.io/firmware/#interrupts-attachinterrupt

CapTouch Touch(D3, D4);

#define PIXEL_COUNT 60
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812

// Parameter 1 = number of pixels in strip
//               note: for some stripes like those with the TM1829, you
//                     need to count the number of segments, i.e. the
//                     number of controllers in your stripe, not the number
//                     of individual LEDs!
// Parameter 2 = pin number (most are valid)
//               note: if not specified, D2 is selected for you.
// Parameter 3 = pixel type [ WS2812, WS2812B, WS2811, TM1803, TM1829 ]
//               note: if not specified, WS2812B is selected for you.
//               note: RGB order is automatically applied to WS2811,
//                     WS2812/WS2812B/TM1803 is GRB order.
//
// 800 KHz bitstream 800 KHz bitstream (most NeoPixel products ...
//                         ... WS2812 (6-pin part)/WS2812B (4-pin part) )
//
// 400 KHz bitstream (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//                   (Radio Shack Tri-Color LED Strip - TM1803 driver
//                    NOTE: RS Tri-Color LED's are grouped in sets of 3)

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);


double lastColorUpdate = 0;     // Epoch of last color update (local or remote)
String colorFromID;             // String, Tracks who sent the color (for debug)
uint16_t colorRecieved;         // 0 - 255, Tracks the color received from another lamp
bool lampOn = 0;                // Tracks if the lamp is lit
uint8_t activeColor = 0;        // 0 - 255, Tracks what color is currently active (default to red)
uint8_t activeR = 255;          // 0 - 255, Red component of activeColor;
uint8_t activeG = 0;            // 0 - 255, Green component of activeColor;
uint8_t activeB = 0;            // 0 - 255, Blue component of activeColor;
uint32_t decayTime = 3600;      // Turn off light after elapsed seconds
uint32_t decayDelay = 5;        // Seconds between decay fade-out
uint32_t decayDelayCounter = 0; // Tracker for decayDelay
int16_t lampBrightness = 0;     // 0 - 255, Tracks current lamp brightness
CapTouch::Event touchEvent;

// Time Zone offset
//int32_t timeZone = -5;

void setup() {
    //Listen for other lamps to send a particle.publish()
    Particle.subscribe("FamiLamp_Update", gotColorUpdate, MY_DEVICES);
    strip.begin();
//    strip.setBrightness(50);
    rainbowFull(5, 0); // 10ms Delay, 0 is fade in //TODO change to longer before finalizing
    rainbowFull(5, 2); // 10ms Delay, 2 is fade out //TODO change to longer before finalizing
    Touch.setup();
}

void loop() {
    touchEvent = Touch.getEvent();

    if (touchEvent == CapTouch::TouchEvent) {
		whileTouching();
	}
    if (Time.now() - lastColorUpdate > decayTime && lampOn == 1) {
        if (decayDelayCounter >= decayDelay) {
            extinguish();
            decayDelayCounter = 0;
        } else {
            decayDelayCounter++;
        }
    }
    //delay(1);
}

void whileTouching() {
    byte activePixels = 0; // Tracks number of active pixels, 0 is first pixel
    byte previousBrightness = lampBrightness; // Store the previous brightness in case we need it later
    uint16_t pixelBrightness = lampBrightness; // Tracks the given pixel's brightness.  Needs to track > 255, so uint16_t
	uint8_t testColor = activeColor; // Start with the current color
    while (touchEvent != CapTouch::ReleaseEvent) {
        for (byte i = 0; i <= activePixels; i++) {
            pixelBrightness = lampBrightness + i;// (i * 5); //Fade to full brightness
			if (pixelBrightness > 255) pixelBrightness = 255; //catch overflow
            // "activePixels - i" reverses the direction
            strip.setPixelColor(activePixels - i, wheelColor(((i * 60 / strip.numPixels()) + testColor) & 255, pixelBrightness)); // "& 255" AKA bitwise and prevents overflow
	    }
        strip.show();
        testColor++; //because testColor is uint8_t, automatically loops at 256
        //+100 is a hack for the pixelBrightness to eventually get to 255 on the last pixel
        if(activePixels < (strip.numPixels() - 1)) activePixels++; //Add 1 for next iteration, but prevent looping around
        if(lampBrightness < 255) lampBrightness++;
        touchEvent = Touch.getEvent();
        delay(3);
    }
	if (activePixels >= (strip.numPixels() - 10)) {
		lampOn = 1;
		activeColor = testColor;
		sendColorUpdate();
		lastColorUpdate = Time.now();
	} else {
	    lampBrightness = previousBrightness;
	    setColor(activeColor);
	}
}

void sendColorUpdate() {
    //char publishString[40];
    String sColor = String(activeColor);
    Particle.publish("FamiLamp_Update", System.deviceID() + "~" + sColor, 60, PRIVATE);
}

void gotColorUpdate(const char *name, const char *data) {
    
    String str = String(data);
    char strBuffer[40] = "";
    str.toCharArray(strBuffer, 40);
    colorFromID = strtok(strBuffer, "~");
    colorRecieved = atof(strtok(NULL, "~"));
    lampBrightness = 255;
    setColor(colorRecieved);
    // DEBUG
    String sColorRecieved = String(colorRecieved);
    Particle.publish("Color_Recieved", System.deviceID() + "~" + sColorRecieved);
    // END DEBUG
	lampOn = 1;
	lastColorUpdate = Time.now();
}

void setColor(byte c) { // c is color.  This function does a downwards fade + wipe to the new color
    uint8_t newR, newG, newB, startR, startG, startB, endR, endG, endB;
    uint32_t color = wheelColor(c, lampBrightness);
    endR = (uint8_t)((color >> 16) & 0xff); // Splits out new color into separate R, G, B
    endG = (uint8_t)((color >> 8) & 0xff);
    endB = (uint8_t)(color & 0xff);
    for (uint16_t fade = 0; fade < 255; fade++) {
        for (uint16_t j = 0; j < strip.numPixels(); j++) {
            long startRGB = strip.getPixelColor(j); // Get pixel's current color
            startR = (uint8_t)((startRGB >> 16) & 0xff); // Splits out current color into separate R, G, B
            startG = (uint8_t)((startRGB >> 8) & 0xff);
            startB = (uint8_t)(startRGB & 0xff);
            //if (j < strip.numPixels()) {
                newR = startR + (endR - startR) * fade / 255;// / strip.numPixels();// Color mixer
                newG = startG + (endG - startG) * fade / 255;// / strip.numPixels();
                newB = startB + (endB - startB) * fade / 255;// / strip.numPixels();
            //} else {
            //    newR = endR;
            //    newG = endG;
            //    newB = endB;
            //}
            strip.setPixelColor(j, newR, newG, newB);
            //strip.setPixelColor(strip.numPixels() - j, color);
        }
        strip.show();
        delay(10);
    }
    activeColor = color;
}

void extinguish() { //Dims the lamp by one unit until lampBrightness is 0 and lampOn is 0
    lampBrightness--;
    uint32_t color = wheelColor(activeColor, lampBrightness);
    for (byte j = 0; j <= strip.numPixels(); j++) {
		strip.setPixelColor(j, color);
    }
    strip.show();
    if (lampBrightness <= 0) {
        lampOn = 0; //If the lamp is completely off, set lampOn to 0
        lampBrightness = 0; // Make sure this number isn't negative somehow
    }
}

uint32_t wheelColor(uint16_t WheelPos, uint16_t iBrightness) {
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
	activeR = R * brightness;// + .5;
	activeG = G * brightness;// + .5;
	activeB = B * brightness;// + .5;
	return strip.Color((byte) activeR,(byte) activeG,(byte) activeB);
}

void rainbowFull(byte wait, byte fade) {
  uint16_t i, j, k;
  if(fade == 0) k = 0;
  else k = 255;

  for(j = 0; j <= 255; j++) {
    for(i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor((strip.numPixels() - 1) - i, wheelColor(((i * 60 / strip.numPixels()) + j) & 255, k));
    }
    strip.show();
    delay(wait);
    if(fade == 0 && k < 255) {
        k++;
    }
    if(fade == 2 && k > 0) {
        k--;
    }
  }
}

void rainbowSingle(uint16_t c, byte j) { //(active color, active pixels)
    uint16_t b; //track brightness for this interation
    for(byte i = 0; i <= j; i++) {
      // "- 10" below backs the color off a bit so that the color shown is a few behind the activeColor
     
      strip.setPixelColor(j - i, wheelColor(((i * 60 / strip.numPixels()) + c - 10) & 255, b)); // "& 255" AKA bitwise and prevents overflow 
    }
    strip.show();
}
