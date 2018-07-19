#include "application.h"
#include "captouch.h"
#include "neopixel/neopixel.h"

SYSTEM_MODE(AUTOMATIC);

CapTouch Touch(D3, D4);

#define PIXEL_COUNT 60
#define PIXEL_PIN D2
#define PIXEL_TYPE WS2812

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN);//, PIXEL_TYPE);

////
// User Variables
////

uint32_t decayTime = 2835;                  // Start extinguishing light after elapsed seconds
uint32_t decayDelay = 15;                    // Seconds between decay fade-out steps
uint8_t nightHours[2] = {6,      21};       // Night mode starts at nightHours[1], ends at nightHours[0]
uint8_t duskHours[2] =  {  7,  19  };       // Dusk mode starts at duskHours[1], ends at duskHours[0].  Needs to be inside nightHours' times.
                                            // Day mode starts at duskHours[0], ends at duskHours[1]
uint16_t maxDayBrightness = 180;            // 0 - 255, lamp will not exceed this during the day
uint16_t maxDuskBrightness = 40;            // 0 - 255, lamp will not exceed this during dusk
uint16_t maxNightBrightness = 3;            // 0 - 255, lamp will not exceed this during the night
uint32_t easterEggRollActivation = 20;      // Activates rainbowEasterEggroll after this many consecutive color changes

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
byte activePixels = 0;                      // Tracks number of active pixels, 0 is first pixel
byte pixelsForPicker = 0;                   // Counter during whileTouching()
uint8_t lastDay = 0;                        // Used to track if onceADay() has run yet today
uint8_t lastHour = 0;                       // Used to track if onceAnHour() has run yet this hour
double touchSenseHeartbeat = 0;             // Epoch of last touch heartbeat
double touchSenseHeartbeatrec = 0;          // Epoch of last touch heartbeat received
uint8_t sparklePixel = 0;                   // Track last pixel touched for sparkle effect

// Variables for special effects
uint32_t consecutiveChanges = 0;            // Track how many times the color has been changed before turning off
uint8_t fadeColor = 0;                      // Track color for special events
uint8_t fadePixelTracker = 0;               // Track last touched pixel for some idle sequences
float redStates[PIXEL_COUNT];               // Fireworks Variable
float blueStates[PIXEL_COUNT];              // Fireworks Variable
float greenStates[PIXEL_COUNT];             // Fireworks Variable
float fadeRate = 0.95;                      // Fireworks Variable: 0.01-0.99, controls decay speed
uint8_t heartbeatDirector = 0;              // Heartbeat Tracking
uint8_t heartbeatColor = 0;                 // Heartbeat Tracking
uint8_t easterEggrollColor = 0;             // Track color for rainbowEasterEggroll()
uint8_t easterMonth;                        // Stores this year's easter month.
uint8_t easterDay;                          // Stores this year's easter day.
CapTouch::Event touchEvent;

void setup() {
    strip.begin();
    strip.show();
    rainbowFull(5, 0); // 5ms Delay, 0 is fade in
    rainbowFull(5, 2); // 5ms Delay, 2 is fade out
    
    Touch.setup();
    
    Time.zone(-5);
    Time.setDSTOffset(1);
    getEasterDate();
    Time.beginDST();
    //Listen for other lamps to send a particle.publish()
    Particle.subscribe("FamiLamp_Update", gotColorUpdate, MY_DEVICES);
    Particle.subscribe("FamiLamp_Sparkle", gotTouchHeartbeat, MY_DEVICES);
}

void loop() {
    // True once every hour
    if (lastHour != Time.hour()) {
        onceAnHour();
        lastHour = Time.hour();
    }
    // True once every day
    if (lastDay != Time.day()) {
        onceADay();
        lastDay = Time.day();
    }
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
    if (Time.now() - touchSenseHeartbeatRec < 5 && touchSenseHeartbeatRec > lastColorUpdate ) {
        touchSparkle();
    } else if ( sparklePixel != strip.numPixels() + 1 ) {
        removeSparkle();
        sparklePixel = strip.numPixels() + 1;
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
        // Halloween
        if (Time.day() == 31 && Time.month() == 10) {
            idleColorFlicker(64);
        }
        // Valentines Day
        if (Time.day() == 14 && Time.month() == 2) {
            idleColorFlicker(106);
        }
        // 4th of July, RWB fireworks
        if ( Time.day() == 4 && Time.month() == 7 ) {
            idleFireworks(2);
        }
        // New Years Day, white fireworks
        if ( Time.day() == 1 && Time.month() == 1 ) {
            idleFireworks(1);
        }
        // Birthdays
        if (
            (Time.day() == 22 && Time.month() == 2) ||
            (Time.day() == 24 && Time.month() == 2)
            ) {
            idleDisco();
        }
        // Easter, relies on onceADay() and getEasterDate()
        if ( Time.day() == easterDay && Time.month() == easterMonth ) {
            //idleDisco();
            idleEaster();
        }
        // Unassigned Heartbeat
        //if (Time.day() == 22 && Time.month() == 10) {
        //    idleHeartbeat();
        //}
        // Unassigned mulitcolored fireworks
        
        /*if ( Time.day() == 1 && Time.month() == 1 ) {
            idleFireworks(0);
        }*/
    }
    // Easter Egg 1
    if (lampOn == 1 && (Time.month() * Time.day()) % 256 == activeColor) {
        rainbowEasterEggroll(0);
    }
    // Easter Egg 2
    if (lampOn == 1 && consecutiveChanges != 0 && consecutiveChanges % easterEggRollActivation == 0) {
        rainbowEasterEggroll(1);
    }
}

void whileTouching() {
    byte previousBrightness = lampBrightness; // Store the previous brightness in case we need it later
    uint16_t pixelBrightness = lampBrightness; // Tracks the given pixel's brightness.  Needs to track > 255, so uint16_t
    uint8_t testColor = activeColor; // Start with the current color
    pixelsForPicker = 0;
    double touchLoopTime
    while (touchEvent != CapTouch::ReleaseEvent) {
        touchLoopTime = Time.now();
        if (touchLoopTime - touchSenseHeartbeat > 3) {
            sendSparkle();
            touchSenseHeartbeat = Time.now();
        }
        for (byte i = 0; i <= pixelsForPicker; i++) {
            pixelBrightness = lampBrightness + i; //Fade to full brightness
            if (pixelBrightness > maxBrightness) pixelBrightness = maxBrightness; //catch overflow
            // "pixelsForPicker - i" reverses the direction
            strip.setPixelColor(pixelsForPicker - i, wheelColor(((i * 60 / strip.numPixels()) + testColor) & 255, pixelBrightness)); // "& 255" AKA bitwise and prevents overflow
        }
        strip.show();
        testColor++; //because testColor is uint8_t, automatically loops at 256
        if(pixelsForPicker < (strip.numPixels() - 1)) pixelsForPicker++; //Add 1 for next iteration, but prevent looping around
        if(lampBrightness < maxBrightness) lampBrightness++;
        touchEvent = Touch.getEvent();
        delay(3);
    }
    if (pixelsForPicker >= (strip.numPixels() - 10)) {
        lampOn = 1;
        activeColor = testColor;
        sendColorUpdate();
        lastColorUpdate = Time.now();
    } else {
        lampBrightness = previousBrightness;
        setColorDither(activeColor);
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
    lampOn = 1;
    consecutiveChanges++;
    activePixels = strip.numPixels();
    setColorDither(colorRecieved);
    // DEBUG
    String sColorRecieved = String(colorRecieved);
    Particle.publish("Color_Recieved", System.deviceID() + "~" + sColorRecieved);
    // END DEBUG
    lastColorUpdate = Time.now();
}

void gotTouchHeartbeat(const char *name, const char *data) {
    String sparkleFromID = String(data);
    if (sparkleFromID == System.deviceID()) {
        return
    } else {
        touchSenseHeartbeatRec = Time.now();
    }
}

void sendSparkle() {
    Particle.publish("FamiLamp_Sparkle", System.deviceID(), 60, PRIVATE);
}

void setColorFade(byte c, byte ) { // c is color.  This function does a smooth fade new color.  Deprecated for new setColorDither()
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
                if (j > activePixels) {
                    newR = startR + (0 - startR) * fade / 255; // Color mixer to black
                    newG = startG + (0 - startG) * fade / 255;
                    newB = startB + (0 - startB) * fade / 255;
                } else {
                    newR = startR + (endR - startR) * fade / 255; // Color mixer
                    newG = startG + (endG - startG) * fade / 255;
                    newB = startB + (endB - startB) * fade / 255;
                }
                strip.setPixelColor(j, newR, newG, newB);
            }
            strip.show();
            delay(10);
        }
    }
    activeColor = c;
}

void setColorDither(byte c) { // c is color.  This function does a "random dither" to set the new color
    // April Fool's day: ignore the color you picked and replace it with something random
    if (Time.day() == 1 && Time.month() == 4) {
        c = random(0,255);
    }
    // Determine highest bit needed to represent pixel index
    uint32_t color = wheelColor(c, lampBrightness);
    int hiBit = 0;
    int n = strip.numPixels() - 1;
    for(int bit=1; bit < 0x8000; bit <<= 1) {
        if(n & bit) hiBit = bit;
    }
    
    int bit, reverse;
    for(int i=0; i<(hiBit << 1); i++) {
        // Reverse the bits in i to create ordered dither:
        reverse = 0;
        for(bit=1; bit <= hiBit; bit <<= 1) {
            reverse <<= 1;
            if(i & bit) reverse |= 1;
        }
        if ( ((Time.month() * Time.day()) % 256) == c) {
            easterEggrollColor = 0;
            color = wheelColor((reverse * 256 / strip.numPixels()) & 255, lampBrightness);
        } else if (consecutiveChanges != 0 && consecutiveChanges % easterEggRollActivation == 0) {
            easterEggrollColor = 0;
            color = wheelColor((reverse * 256 / 6) & 255, lampBrightness);
        }
        if (reverse > activePixels) {
            strip.setPixelColor(reverse, 0, 0, 0);
        } else {
            strip.setPixelColor(reverse, color);
        }
        strip.show();
        delay(20);
    }
    activeColor = c;
}

void extinguishFade() { //Dims the lamp by one unit until lampBrightness is 0 and lampOn is 0
    lampBrightness--;
    if (((Time.month() * Time.day()) % 256) == activeColor) { // Semi-random formula to trigger easter egg
        rainbowEasterEgg();
    } else {
        uint32_t color = wheelColor(activeColor, lampBrightness);
        for (byte j = 0; j <= strip.numPixels(); j++) {
            strip.setPixelColor(j, color);
        }
        strip.show();
        if (lampBrightness <= 0) {
            lampOn = 0; //If the lamp is completely off, set lampOn to 0
            consecutiveChanges = 0; //Reset counter for RainbowEasterEggroll() activation
            lampBrightness = 0; // Make sure this number isn't negative somehow
        }
    }
}

void extinguish() { //Turns off one pixel at a time until they're all off
    activePixels--;
    for(int i = 0; i <= strip.numPixels() - activePixels; i++) {
        strip.setPixelColor(strip.numPixels() - i, 0, 0, 0);
    }
    strip.show();
    if (activePixels <= 0) {
            lampOn = 0; //If the lamp is completely off, set lampOn to 0
            consecutiveChanges = 0; //Reset counter for RainbowEasterEggroll() activation
            lampBrightness = 0;
            activePixels = 0; // Make sure this number isn't negative somehow
        }
}

void extinguishOld() { //Dims the lamp by one unit until lampBrightness is 0 and lampOn is 0
    lampBrightness--;
    uint32_t color = wheelColor(activeColor, lampBrightness);
    for (byte j = 0; j <= strip.numPixels(); j++) {
        strip.setPixelColor(j, color);
    }
    strip.show();
    if (lampBrightness <= 0) {
        lampOn = 0; //If the lamp is completely off, set lampOn to 0
        consecutiveChanges = 0; //Reset counter for RainbowEasterEggroll() activation
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
  uint16_t thisColor;
  if(fade == 0) k = 0;
  else k = maxBrightness;

  for(j = 0; j <= 255; j++) {
    for(i = 0; i < strip.numPixels(); i++) {
        thisColor = wheelColor(((i * 60 / strip.numPixels()) + j) & 255, k);
        strip.setPixelColor((strip.numPixels() - 1) - i, thisColor);
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

void removeSparkle() {
    // Sparkle the lamp when another lamp is being touched
    if ( ((Time.month() * Time.day()) % 256) == c) {
        uint32_t color = wheelColor((sparklePixel * 256 / strip.numPixels()) & 255, lampBrightness);
    } else if (consecutiveChanges != 0 && consecutiveChanges % easterEggRollActivation == 0) {
        uint32_t color = wheelColor((sparklePixel * 256 / 6) & 255, lampBrightness);
    } else {
        uint32_t color = wheelColor(activeColor, lampBrightness);
    }
    strip.setPixelColor(sparklePixel, color);
    strip.show();
}

void touchSparkle() {
    removeSparkle();
    if (random(10) == 1) {
        sparklePixel = random(strip.numPixels());
        strip.setPixelColor(sparklePixel, maxBrightness, maxBrightness, maxBrightness);
    }
    strip.show();
}

void onceADay() {
    // Clear any previous day's special idles
    if (lampBrightness != 0 && lampOn == 0) {
        for (uint16_t i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, 0, 0, 0);
        }
        strip.show();
        lampBrightness = 0;
    }
    getEasterDate();
}

void onceAnHour() {
    dayTracking();
}

void getEasterDate() {
    // Getting the date of easter is a giant pain.
    easterDay = (19 * (Time.year() % 19) + 24) % 30;        
    easterDay = 22 + easterDay + ((2 * (Time.year() % 4) + 4 * (Time.year() % 7) + 6 * easterDay + 5) % 7);
    // jump to next month
    if( easterDay > 31 ) {
        easterMonth = 4;
        easterDay -= 31;
    } else {
        easterMonth = 3;
    }
    // DEBUG
    String sEasterDay = String(easterDay);
    String sEasterMonth = String(easterMonth);
    String sTodayDay = String(Time.day());
    String sTodayMonth = String(Time.month());
    Particle.publish("This is Easter", "Month: " + sEasterMonth + ", Day: " + sEasterDay);
    Particle.publish("Today is", "Month: " + sTodayMonth + ", Day: " + sTodayDay);
    // END DEBUG
}

void rainbowEasterEgg() {
    // displays full rainbow, deprecated function now included in setColorDither() function
    for(uint8_t i = 0; i <= activePixels; i++) {
        strip.setPixelColor(i, wheelColor((i * 256 / strip.numPixels()) & 255, lampBrightness));
    }
    strip.show();
}

void rainbowEasterEggroll(byte type) {
    // displays full rainbow and rolls the color each time called
    uint16_t magicNumber;
    if (type == 1) {
        magicNumber = 6;
    } else if (type == 2) {
        magicNumber = 10;
    } else if (type == 3) {
        magicNumber = 12;
    } else {
        magicNumber = strip.numPixels();
    }
    for(uint8_t i = 0; i < activePixels; i++) {
        strip.setPixelColor(i, wheelColor(((i * 256 / magicNumber) + easterEggrollColor) & 255, lampBrightness));
    }
    strip.show();
    delay(10);
    easterEggrollColor++;
}

void dayTracking() {
    if (Time.hour() < nightHours[0] || Time.hour() >= nightHours[1]) { // Night hours
        if (dayTrack != 2) {
            maxBrightness = maxNightBrightness;
            if (lampBrightness > maxBrightness) lampBrightness = maxBrightness;
            setColorDither(activeColor);
            dayTrack = 2;
        }
    } else if (Time.hour() < duskHours[0] || Time.hour() >= duskHours[1]) { // Dusk hours
        if (dayTrack != 1) {
            maxBrightness = maxDuskBrightness;
            if (lampBrightness > maxBrightness) lampBrightness = maxBrightness;
            setColorDither(activeColor);
            dayTrack = 1;
        }
    } else { // Everything else is day
        if (dayTrack != 0) {
            maxBrightness = maxDayBrightness;
            if (lampBrightness > maxBrightness) lampBrightness = maxBrightness;
            setColorDither(activeColor);
            dayTrack = 0;
        }
    }
}

void idleColorFader(uint8_t c1, uint8_t c2) {
    // Slow fade between the two specified colors, `c1` and `c2`
    lampBrightness = 40;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
    uint16_t currR, currG, currB, endR, endG, endB;
    uint32_t color = wheelColor(fadeColor, lampBrightness);
    endR = (uint16_t)((color >> 16) & 0xff); // Splits out new color into separate R, G, B
    endG = (uint16_t)((color >> 8) & 0xff);
    endB = (uint16_t)(color & 0xff);
    for (uint16_t j = 0; j < fadePixelTracker; j++) {
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
            fadePixelTracker = 0;
        }
    }
    strip.show();
    if ( fadePixelTracker < strip.numPixels() ) fadePixelTracker++;
    delay(20);
}

void idleFireworks(uint8_t w) {
    // Emulates fireworks bursting inside lamp.  Single LEDs flash
    // in the specified color pattern:
    // `w = 0` for mulitcolor, `w = 1` for all white flashes, 'w = 2` for red, white, and blue
    lampBrightness = 40;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
    if (random(20) == 1) {
        uint16_t i = random(strip.numPixels());
        if (redStates[i] < 1 && greenStates[i] < 1 && blueStates[i] < 1) {
            if (w == 0){
                redStates[i] = random(lampBrightness);
                greenStates[i] = random(lampBrightness);
                blueStates[i] = random(lampBrightness);
            } else if (w == 1) {
                redStates[i] = lampBrightness;
                greenStates[i] = lampBrightness;
                blueStates[i] = lampBrightness;
            } else {
                uint8_t j = random(3);
                if (j == 0) {
                    redStates[i] = lampBrightness;
                    greenStates[i] = 0;
                    blueStates[i] = 0;
                } else if (j == 1) {
                    redStates[i] = lampBrightness;
                    greenStates[i] = lampBrightness;
                    blueStates[i] = lampBrightness;
                } else {
                    redStates[i] = 0;
                    greenStates[i] = 0;
                    blueStates[i] = lampBrightness;
                }
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
    // Recreation of 70s Disco floor.  Each cycle 60 random LEDs are updated
    // to 60 random colors and brightnesses.
    lampBrightness = 20;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
    for(int i=0; i<strip.numPixels(); i++) {
        int randr = random(0,lampBrightness);
        int randg = random(0,lampBrightness); 
        int randb = random(0,lampBrightness);
        int randi = random(0,strip.numPixels());
        strip.setPixelColor(randi, randr, randg, randb);
        strip.show();
        delay(5);
    }
}
void idleColorFlicker(uint8_t c) {
    // Similar to idleDisco, but only uses a single color and randomly
    // varies brightness between `lampBrightness` and `lampBrightness - 10`
    lampBrightness = 20;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
    uint32_t color = wheelColor(c, lampBrightness);
    for(uint8_t i=0; i<strip.numPixels(); i++) {    
        uint8_t j = random(0,strip.numPixels()-1);
        uint8_t flicker = random(0,10);
        int flickerR = (uint16_t)((color >> 16) & 0xff) - flicker; // Splits out new color into separate R, G, B
        int flickerG = (uint16_t)((color >> 8) & 0xff) - flicker;
        int flickerB = (uint16_t)(color & 0xff) - flicker;
        if(flickerR<0) flickerR=0;
        if(flickerG<0) flickerG=0;
        if(flickerB<0) flickerB=0;
        strip.setPixelColor(j, flickerR, flickerG, flickerB);
        
    }
    strip.show();
    delay(20);
}
void idleHeartbeat() {
    lampBrightness = 20;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
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

void idleEaster() {
    lampBrightness = 20;
    if ( maxBrightness < lampBrightness ) {
        lampBrightness = maxBrightness;
    }
    activePixels = strip.numPixels();
    rainbowEasterEggroll(2);
}