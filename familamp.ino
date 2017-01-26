#include "application.h"
#include "captouch.h"
#include "neopixel/neopixel.h"

SYSTEM_MODE(AUTOMATIC);

//PRODUCT_ID and PRODUCT_VERSION required for Particle's "Products" feature
PRODUCT_ID(639);
PRODUCT_VERSION(12);

CapTouch Touch(D3, D4);

#define PIXEL_COUNT 60
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

////
// User Variables
////

uint32_t decayTime = 2835;                  // Start dimming light after elapsed seconds
uint32_t decayDelay = 3;                    // Seconds between decay fade-out steps
uint8_t nightHours[2] = {6,      21};       // Night mode starts at nightHours[1], ends at nightHours[0]
uint8_t duskHours[2] =  {  7,  19  };       // Dusk mode starts at duskHours[1], ends at duskHours[0].  Needs to be inside nightHours' times.
                                            // Day mode starts at duskHours[0], ends at duskHours[1]
uint16_t maxDayBrightness = 180;            // 0 - 255, lamp will not exceed this during the day
uint16_t maxDuskBrightness = 70;            // 0 - 255, lamp will not exceed this during dusk
uint16_t maxNightBrightness = 30;           // 0 - 255, lamp will not exceed this during the night
float fadeRate = 0.95;                      // Fireworks Variable: 0.01-0.99, controls decay speed

////
// End User Variables
////

double lastColorUpdate = 0;                 // Epoch of last color update (local or remote)
String colorFromID;                         // String, Tracks who sent the color (for debug)
uint16_t colorRecieved;                     // 0 - 255, Tracks the color received from another lamp
bool lampOn = 0;                            // Tracks if the lamp is lit
uint8_t activeColor = 0;                    // 0 - 255, Tracks what color is currently active (start at red)
uint8_t activeR = 255;                      // 0 - 255, Red component of activeColor;
uint8_t activeG = 0;                        // 0 - 255, Green component of activeColor;
uint8_t activeB = 0;                        // 0 - 255, Blue component of activeColor;
double lastDecayDelay = 0;                  // Time Tracker for decayDelay
uint16_t lampBrightness = 0;                // 0 - 255, Tracks current lamp brightness
uint16_t maxBrightness = maxDayBrightness;  // Assigned the current max brightness
uint8_t dayTrack = 0;                       // Track day/dusk/night condition
uint8_t fadeColor = 0;                      // Track color for special events
byte activePixels = 0;                      // Tracks number of active pixels, 0 is first pixel
float redStates[PIXEL_COUNT];               // Fireworks Variable
float blueStates[PIXEL_COUNT];              // Fireworks Variable
float greenStates[PIXEL_COUNT];             // Fireworks Variable
uint8_t heartbeatDirector = 0;              // Heartbeat Tracking
uint8_t heartbeatColor = 0;                 // Heartbeat Tracking
CapTouch::Event touchEvent;

void setup() {
    strip.begin();
    strip.show();
    rainbowFull(5, 0); // 5ms Delay, 0 is fade in
    rainbowFull(5, 2); // 5ms Delay, 2 is fade out
    
    Touch.setup();
    
    Time.zone(-5);
    //Listen for other lamps to send a particle.publish()
    Particle.subscribe("FamiLamp_Update", gotColorUpdate, MY_DEVICES);
}

void loop() {
    dayTracking();
    touchEvent = Touch.getEvent();

    if (touchEvent == CapTouch::TouchEvent) {
		whileTouching();
	}
    if (Time.now() - lastColorUpdate > decayTime && lampOn == 1) {
        if (Time.now() - lastDecayDelay >= decayDelay) {
            extinguish();
            lastDecayDelay = Time.now();
        }
    }
    
    // Special idle functions
    if (lampOn == 0) {
        // Christmas Day
        if (Time.day() == 25 && Time.month() == 12) {
            idleColorFader(0,85);
        }
        // St. Patricks Day
        if (Time.day() == 17 && Time.month() == 3) {
            idleColorFlicker(21);
        }
        // Valentines Day
        if (Time.day() == 14 && Time.month() == 2) {
            idleColorFlicker(106);
        }
        // 4th of July
        if ( Time.day() == 4 && Time.month() == 7 ) {
            idleFireworks(0);
        }
        // New Years Day
        if ( Time.day() == 1 && Time.month() == 1 ) {
            idleFireworks(1);
        }
        // Birthdays
        if (
            (Time.day() == 22 && Time.month() == 2)
            ) {
            idleDisco();
        }
        // Clear any previous day's special idles
        if (lampBrightness != 0 && Time.hour() == 0 && Time.minute() == 0 && Time.second() <= 3) {
            for (uint16_t i = 0; i < strip.numPixels(); i++) {
                strip.setPixelColor(i, 0, 0, 0);
            }
            strip.show();
            lampBrightness = 0;
        }
    }
}

void whileTouching() {
    byte previousBrightness = lampBrightness; // Store the previous brightness in case we need it later
    uint16_t pixelBrightness = lampBrightness; // Tracks the given pixel's brightness.  Needs to track > 255, so uint16_t
	uint8_t testColor = activeColor; // Start with the current color
    activePixels = 0;
    while (touchEvent != CapTouch::ReleaseEvent) {
        for (byte i = 0; i <= activePixels; i++) {
            pixelBrightness = lampBrightness + i; //Fade to full brightness
			if (pixelBrightness > maxBrightness) pixelBrightness = maxBrightness; //catch overflow
            // "activePixels - i" reverses the direction
            strip.setPixelColor(activePixels - i, wheelColor(((i * 60 / strip.numPixels()) + testColor) & 255, pixelBrightness)); // "& 255" AKA bitwise and prevents overflow
	    }
        strip.show();
        testColor++; //because testColor is uint8_t, automatically loops at 256
        if(activePixels < (strip.numPixels() - 1)) activePixels++; //Add 1 for next iteration, but prevent looping around
        if(lampBrightness < maxBrightness) lampBrightness++;
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
    String sColor = String(activeColor);
    Particle.publish("FamiLamp_Update", System.deviceID() + "~" + sColor, 60, PRIVATE);
}

void gotColorUpdate(const char *name, const char *data) {

    String str = String(data);
    char strBuffer[40] = "";
    str.toCharArray(strBuffer, 40);
    colorFromID = strtok(strBuffer, "~");
    colorRecieved = atof(strtok(NULL, "~"));
    lampBrightness = maxBrightness;
    setColor(colorRecieved);
    // DEBUG
    String sColorRecieved = String(colorRecieved);
    Particle.publish("Color_Recieved", System.deviceID() + "~" + sColorRecieved);
    // END DEBUG
	lampOn = 1;
	lastColorUpdate = Time.now();
}

void setColor(byte c) { // c is color.  This function does a smooth fade new color
    if (((Time.month() * Time.day()) % 256) == c) { // Semi-random formula to trigger easter egg
        rainbowEasterEgg();
    } else {
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
                newR = startR + (endR - startR) * fade / 255; // Color mixer
                newG = startG + (endG - startG) * fade / 255;
                newB = startB + (endB - startB) * fade / 255;
                strip.setPixelColor(j, newR, newG, newB);
            }
            strip.show();
            delay(10);
        }
    }
    activeColor = c;
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
  else k = maxBrightness;

  for(j = 0; j <= 255; j++) {
    for(i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor((strip.numPixels() - 1) - i, wheelColor(((i * 60 / strip.numPixels()) + j) & 255, k));
    }
    strip.show();
    delay(wait);
    if(fade == 0 && k < maxBrightness) {
        k++;
    }
    if(fade == 2 && k > 0) {
        k--;
    }
  }
}

void rainbowEasterEgg() {
    for(uint8_t i = 0; i <= strip.numPixels(); i++) {
      strip.setPixelColor(i, wheelColor((i * 256 / strip.numPixels()) & 255, lampBrightness));
    }
    strip.show();
}

void dayTracking() {
    if (Time.hour() < nightHours[0] || Time.hour() >= nightHours[1]) { // Night hours
        if (dayTrack != 2) {
            maxBrightness = maxNightBrightness;
            if (lampBrightness > maxBrightness) lampBrightness = maxBrightness;
            setColor(activeColor);
            dayTrack = 2;
        }
    } else if (Time.hour() < duskHours[0] || Time.hour() >= duskHours[1]) { // Dusk hours
        if (dayTrack != 1) {
            maxBrightness = maxDuskBrightness;
            if (lampBrightness > maxBrightness) lampBrightness = maxBrightness;
            setColor(activeColor);
            dayTrack = 1;
        }
    } else { // Everything else is day
        if (dayTrack != 0) {
            maxBrightness = maxDayBrightness;
            if (lampBrightness > maxBrightness) lampBrightness = maxBrightness;
            setColor(activeColor);
            dayTrack = 0;
        }
    }
}

void idleColorFader(uint8_t c1, uint8_t c2) {
    lampBrightness = 100;
    uint16_t currR, currG, currB, endR, endG, endB;
    uint32_t color = wheelColor(fadeColor, lampBrightness);
    endR = (uint16_t)((color >> 16) & 0xff); // Splits out new color into separate R, G, B
    endG = (uint16_t)((color >> 8) & 0xff);
    endB = (uint16_t)(color & 0xff);
    for (uint16_t j = 0; j < activePixels; j++) {
        long startRGB = strip.getPixelColor(j); // Get pixel's current color
        currR = (uint16_t)((startRGB >> 16) & 0xff); // Splits out current color into separate R, G, B
        currG = (uint16_t)((startRGB >> 8) & 0xff);
        currB = (uint16_t)(startRGB & 0xff);
        if ( currR > endR ) {
            currR = currR - 10;
        } else if ( currR < endR ) {
            currR = currR + 10;
        } else {
            currR = endR;
        }
        if ( currG > endG ) {
            currG = currG - 10;
        } else if ( currG < endG ) {
            currG = currG + 10;
        } else {
            currG = endG;
        }
        if ( currB > endB ) {
            currB = currB - 10;
        } else if ( currB < endB ) {
            currB = currB + 10;
        } else {
            currB = endB;
        }
        
        //Catch overflows
        currR %= 255;
        currG %= 255;
        currB %= 255;

        strip.setPixelColor(j, currR, currG, currB);
void whileTouching() {
    byte previousBrightness = lampBrightness; // Store the previous brightness in case we need it later
    uint16_t pixelBrightness = lampBrightness; // Tracks the given pixel's brightness.  Needs to track > 255, so uint16_t
	uint8_t testColor = activeColor; // Start with the current color
    activePixels = 0;
    while (touchEvent != CapTouch::ReleaseEvent) {
        for (byte i = 0; i <= activePixels; i++) {
            pixelBrightness = lampBrightness + i; //Fade to full brightness
			if (pixelBrightness > maxBrightness) pixelBrightness = maxBrightness; //catch overflow
            // "activePixels - i" reverses the direction
            strip.setPixelColor(activePixels - i, wheelColor(((i * 60 / strip.numPixels()) + testColor) & 255, pixelBrightness)); // "& 255" AKA bitwise and prevents overflow
	    }
        strip.show();
        testColor++; //because testColor is uint8_t, automatically loops at 256
        if(activePixels < (strip.numPixels() - 1)) activePixels++; //Add 1 for next iteration, but prevent looping around
        if(lampBrightness < maxBrightness) lampBrightness++;
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
    String sColor = String(activeColor);
    Particle.publish("FamiLamp_Update", System.deviceID() + "~" + sColor, 60, PRIVATE);
}

void gotColorUpdate(const char *name, const char *data) {

    String str = String(data);
    char strBuffer[40] = "";
    str.toCharArray(strBuffer, 40);
    colorFromID = strtok(strBuffer, "~");
    colorRecieved = atof(strtok(NULL, "~"));
    lampBrightness = maxBrightness;
    setColor(colorRecieved);
    // DEBUG
    String sColorRecieved = String(colorRecieved);
    Particle.publish("Color_Recieved", System.deviceID() + "~" + sColorRecieved);
    // END DEBUG
	lampOn = 1;
	lastColorUpdate = Time.now();
}

void setColor(byte c) { // c is color.  This function does a smooth fade new color
    if (((Time.month() * Time.day()) % 256) == c) { // Semi-random formula to trigger easter egg
        rainbowEasterEgg();
    } else {
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
                newR = startR + (endR - startR) * fade / 255; // Color mixer
                newG = startG + (endG - startG) * fade / 255;
                newB = startB + (endB - startB) * fade / 255;
                strip.setPixelColor(j, newR, newG, newB);
            }
            strip.show();
            delay(10);
        }
    }
    activeColor = c;
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
  else k = maxBrightness;

  for(j = 0; j <= 255; j++) {
    for(i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor((strip.numPixels() - 1) - i, wheelColor(((i * 60 / strip.numPixels()) + j) & 255, k));
    }
    strip.show();
    delay(wait);
    if(fade == 0 && k < maxBrightness) {
        k++;
    }
    if(fade == 2 && k > 0) {
        k--;
    }
  }
}

void rainbowEasterEgg() {
    for(uint8_t i = 0; i <= strip.numPixels(); i++) {
      strip.setPixelColor(i, wheelColor((i * 256 / strip.numPixels()) & 255, lampBrightness));
    }
    strip.show();
}

void dayTracking() {
    if (Time.hour() < nightHours[0] || Time.hour() >= nightHours[1]) { // Night hours
        if (dayTrack != 2) {
            maxBrightness = maxNightBrightness;
            if (lampBrightness > maxBrightness) lampBrightness = maxBrightness;
            setColor(activeColor);
            dayTrack = 2;
        }
    } else if (Time.hour() < duskHours[0] || Time.hour() >= duskHours[1]) { // Dusk hours
        if (dayTrack != 1) {
            maxBrightness = maxDuskBrightness;
            if (lampBrightness > maxBrightness) lampBrightness = maxBrightness;
            setColor(activeColor);
            dayTrack = 1;
        }
    } else { // Everything else is day
        if (dayTrack != 0) {
            maxBrightness = maxDayBrightness;
            if (lampBrightness > maxBrightness) lampBrightness = maxBrightness;
            setColor(activeColor);
            dayTrack = 0;
        }
    }
}

void idleColorFader(uint8_t c1, uint8_t c2) {
    lampBrightness = 40;
    uint16_t currR, currG, currB, endR, endG, endB;
    uint32_t color = wheelColor(fadeColor, lampBrightness);
    endR = (uint16_t)((color >> 16) & 0xff); // Splits out new color into separate R, G, B
    endG = (uint16_t)((color >> 8) & 0xff);
    endB = (uint16_t)(color & 0xff);
    for (uint16_t j = 0; j < activePixels; j++) {
        long startRGB = strip.getPixelColor(j); // Get pixel's current color
        currR = (uint16_t)((startRGB >> 16) & 0xff); // Splits out current color into separate R, G, B
        currG = (uint16_t)((startRGB >> 8) & 0xff);
        currB = (uint16_t)(startRGB & 0xff);
        if ( currR > endR ) {
            currR = currR - 1;
        } else if ( currR < endR ) {
            currR = currR + 1;
        } else {
            currR = endR;
        }
        if ( currG > endG ) {
            currG = currG - 1;
        } else if ( currG < endG ) {
            currG = currG + 1;
        } else {
            currG = endG;
        }
        if ( currB > endB ) {
            currB = currB - 1;
        } else if ( currB < endB ) {
            currB = currB + 1;
        } else {
            currB = endB;
        }
        
        //Catch overflows
        currR %= 255;
        currG %= 255;
        currB %= 255;

        strip.setPixelColor(j, currR, currG, currB);
        if ( j >= strip.numPixels() - 1 && endR == currR && endG == currG && endB == currB) {
            if ( fadeColor == c1 ) {
                fadeColor = c2;
            } else {
                fadeColor = c1;
            }
            activePixels = 0;
        }
    }
    strip.show();
    if ( activePixels < strip.numPixels() ) activePixels++;
    delay(20);
}

void idleFireworks(uint8_t w) {
    // w = 0 for mulitcolor, w = 1 for all white flashes
    lampBrightness = 40;
    if (random(20) == 1) {
        uint16_t i = random(strip.numPixels());
        if (redStates[i] < 1 && greenStates[i] < 1 && blueStates[i] < 1) {
            if (w == 0){
                redStates[i] = random(lampBrightness);
                greenStates[i] = random(lampBrightness);
                blueStates[i] = random(lampBrightness);
            } else {
                redStates[i] = lampBrightness;
                greenStates[i] = lampBrightness;
                blueStates[i] = lampBrightness;
            }
        }
    }
    for(uint16_t l = 0; l < strip.numPixels(); l++) {
        if (redStates[l] > 1 || greenStates[l] > 1 || blueStates[l] > 1) {
            strip.setPixelColor(l, redStates[l], greenStates[l], blueStates[l]);
            if (redStates[l] > 1) {
                redStates[l] = redStates[l] * fadeRate;
            } else {
                redStates[l] = 0;
            }
        
            if (greenStates[l] > 1) {
                greenStates[l] = greenStates[l] * fadeRate;
            } else {
                greenStates[l] = 0;
            }
        
            if (blueStates[l] > 1) {
                blueStates[l] = blueStates[l] * fadeRate;
            } else {
                blueStates[l] = 0;
            }
        
        } else {
            strip.setPixelColor(l, 0, 0, 0);
        }
    }
    strip.show();
}
void idleDisco() {
    lampBrightness = 20;
    for(int i=0; i<strip.numPixels(); i++) {
        int randr = random(0,lampBrightness);
        int randg = random(0,lampBrightness); 
        int randb = random(0,lampBrightness);
        int randi = random(0,(strip.numPixels() - 1));
        strip.setPixelColor(randi, randr, randg, randb);
        strip.show();
        delay(5);
    }
}
void idleColorFlicker(uint8_t c) {
    lampBrightness = 20;
    uint32_t color = wheelColor(c, lampBrightness);
    
    for(uint8_t i=0; i<strip.numPixels(); i++) {
        uint8_t flicker = random(0,10);
        int flickerR = (uint16_t)((color >> 16) & 0xff) - flicker; // Splits out new color into separate R, G, B
        int flickerG = (uint16_t)((color >> 8) & 0xff) - flicker;
        int flickerB = (uint16_t)(color & 0xff) - flicker;
        if(flickerR<0) flickerR=0;
        if(flickerG<0) flickerG=0;
        if(flickerB<0) flickerB=0;
        strip.setPixelColor(i, flickerR, flickerG, flickerB);
    }
    strip.show();
    delay(30);
}
void idleHeartbeat() {
    lampBrightness = 20;
    uint8_t endColor = 0;
    
    if( heartbeatDirector == 0 ) {
        endColor = lampBrightness * 0.6;
    }else if( heartbeatDirector == 1 ) {
        endColor = lampBrightness * 0.2;
    }else if( heartbeatDirector == 2 ) {
        endColor = lampBrightness;
    }else if( heartbeatDirector == 3 ) {
        endColor = lampBrightness * 0.12;
    } else {
        //do nothing, this will delay
    }

    if( heartbeatColor < endColor ) {
        for(int j=heartbeatColor; j<endColor; j+=4) {
            for(int i=25; i<35; i++) {
                strip.setPixelColor(i, j, 0, 0);
                
            }
        strip.show();
        delay(15);
        }
    } else if ( heartbeatColor > endColor ) {
        for(int j=heartbeatColor; j>endColor; j--) {
            for(int i=25; i<35; i++) {
                strip.setPixelColor(i, j, 0, 0);
                
            }
        strip.show();
        delay(30);
        }
    } else {
        delay(15);
        delay(15);
        delay(15);
        delay(15);
        delay(15);
        delay(15);
        delay(15);
    }
    
    heartbeatColor = endColor;
    
    heartbeatDirector++;
    heartbeatDirector%=4;
}
