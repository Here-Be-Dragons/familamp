//#include "application.h"
#include "captouch.h"
#include "neopixel/neopixel.h"

SYSTEM_MODE(AUTOMATIC);

// Define which pins are connected with a 1-10M resistor.
// The first pin will be connected to the touch sensor
// and must be D0, D1, D2, D3, D4 A0, A1, A3, A4, A5, A6, A7
// see: http://docs.spark.io/firmware/#interrupts-attachinterrupt

CapTouch Touch(D3, D4);

#define PIXEL_COUNT 16
#define PIXEL_PIN D0
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


double lastColorUpdate = 0;       //Tracks when we last got sent a color
uint32_t colorFromID;             //Tracks who sent the color
uint32_t colorRecieved;           //Tracks the color recieved from another lamp
uint32_t activeColor;             //Tracks what color is currently active
uint32_t decayTime = 300;         //Turn off light after elapsed milliseconds
CapTouch::Event touchEvent;

// Time Zone offset
int32_t timeZone = -5;

void setup() {
    Touch.setup();
    //Listen for other lamps to send a particle.publish()
    Particle.subscribe("aaa", gotColorUpdate);
    strip.begin();
    /*for (int i = 0; i < strip.numPixels(); i++){
        strip.setPixelColor(i, 255, 255, 255);
    }
    strip.show();
    for (int j = 255; j >= 0; j--){
        for (int i = 0; i < strip.numPixels(); i++){
            strip.setPixelColor(i, j, j, j);
        }
        strip.show();
        delay(5);
    }*/
    rainbow(30, 0); //0 is fade in
    //for(uint16_t i = 0; i < 3; i++) {
        rainbow(30, 1); //1 is no fade
    //}
    rainbow(30, 2); //2 is fade out
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
        uint32_t color = wheelColor(i,255);
        activeColor = setColor(color);
        i++;
        i %= 255;
        delay(1);
        touchEvent = Touch.getEvent();
    }
}

void sendColorUpdate(uint32_t c) {
    char publishString[40];
    //sprintf(publishString, "%u", c);
    String sColor = String(c);
    Particle.publish("aaa", System.deviceID() + "~" + sColor);
    lastColorUpdate = Time.now();
}

void gotColorUpdate(const char *name, const char *data) {
    
    String str = String(data);
    char strBuffer[40] = "";
    str.toCharArray(strBuffer, 40);
    lastColorUpdate = Time.now();
    //colorRecieved = atof(strtok(strBuffer, "~"));
    colorFromID = atof(strtok(strBuffer, "~"));
    colorRecieved = atof(strtok(NULL, "~"));
    activeColor = setColor(colorRecieved);
    String sColorFromID = String(colorFromID);
    String sColorRecieved = String(colorRecieved);
    Particle.publish("VarRec", sColorFromID + "~" + sColorRecieved);
}

uint32_t setColor(uint32_t c) {
    for (byte j = 0; j < strip.numPixels(); j++) {
		strip.setPixelColor(j, c);
    }
    strip.show();
    return c;
}

/*uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}*/

uint32_t wheelColor(byte WheelPos, byte iBrightness) {
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
}

void rainbow(uint8_t wait, uint8_t fade) {
  uint16_t i, j, k;
  if(fade == 0) k = 0;
  else k = 255;

  for(j=0; j<=255; j+=5) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, wheelColor(((i * 600 / strip.numPixels()) + j) & 255, k));
    }
    strip.show();
    delay(wait);
    if(fade == 0) {
          if( k < 255 ) k++;
    }
    if (fade == 2) {
          if( k > 0 ) k--;
    }
  }
}
