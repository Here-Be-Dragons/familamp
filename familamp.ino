// This #include statement was automatically added by the Particle IDE.
#include "Adafruit_SSD1306/Adafruit_SSD1306.h"

// This #include statement was automatically added by the Particle IDE.
#include "neopixel/neopixel.h"

// TWEAKABLE VALUES FOR CAP SENSING. THE BELOW VALUES WORK WELL AS A STARTING PLACE:
#define BASELINE_VARIANCE 512.0 // the higher the number the less the baseline is affected by current readings. (was 4)
#define SENSITIVITY 8 // Integer. Higher is more sensitive (was 8)
#define BASELINE_SENSITIVITY 16 // Integer. This is a trigger point such that values exceeding this point will not affect the baseline. Higher values make the trigger point sooner. (was 16)
#define SAMPLE_SIZE 512 // 512 // Number of samples to take for one reading. Higher is more accurate but large values cause some latency.(was 32)
#define SAMPLES_BETWEEN_PIXEL_UPDATES 32
#define LOOPS_TO_FINAL_COLOR 150

// STATES:
#define PRE_ATTACK 0
#define ATTACK 1
#define DECAY 2
#define SUSTAIN 3
#define RELEASE1 4
#define RELEASE2 5
#define OFF 6

#define tEVENT_NONE 0
#define tEVENT_TOUCH 1
#define tEVENT_RELEASE 2

#define END_VALUE 0
#define TIME 1

// END VALUE, TIME
// 160 is approximately 1 second
const long envelopes[6][2] = {
{0, 0} ,          // OFF
{255, 30} ,      // ATTACK
{200, 240},     // DECAY
{200, 1000},       // SUSTAIN
{150, 60},     // RELEASE1
{0, 1000000}      // RELEASE2 (65535 is about 6'45")
};

// NEOPIXEL
#define PIXEL_PIN D6
#define PIXEL_COUNT 30
#define PIXEL_TYPE WS2812B

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

int sPin = D4; //Touch Send Pin
int rPin = D1; //Touch Receive Pin
int i = 0; //Come up with a better name... this cycles color
uint32_t color = 0; //Track color
int currentEvent = tEVENT_NONE;
unsigned char state = OFF;

// timestamps
unsigned long tS;
volatile unsigned long tR;

// reading and baseline
long tReading;
float tBaseline;
int touchEvent;

void setup() {
    //Initialize LEDs
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    strip.setBrightness(40);
    
    //Configure sPin for output
    pinMode(sPin,OUTPUT);
    pinMode(rPin, INPUT); //For early testing
    attachInterrupt(rPin,touchSense,RISING);
    Particle.variable("color", color);
    Particle.variable("tBaseline", tBaseline);
    Particle.variable("touchEvent", touchEvent);

    
    
    for (i = 0; i <= 255; i++) {
        color = wheelColor(i,255);
        for (int j = 0; j < strip.numPixels(); j++) {
            strip.setPixelColor(j, color);
        }
        strip.show();
        //delay(10);
    }
    for (int j = 255; j >= 0; j--) {
        color = wheelColor(255, j);
        for (int k = 0; k < strip.numPixels(); k++) {
            strip.setPixelColor(k, color);
        }
        strip.show();
        //delay(10);
        i = rand() % 255;
    }
    //Trigger an interrupt when the Lamp is touched
    
   
    Particle.subscribe("FamiLamp",commReceived);
    // calibrate touch sensor
    tBaseline = touchSampling();    // initialize to first reading
}

void loop() {
    touchEvent = touchEventCheck();
    if (touchEvent == tEVENT_TOUCH) {
        for (int k = 0; k < strip.numPixels(); k++) {
            strip.setPixelColor(k, 0, 0, 255);
        }
        strip.show();
        //currentEvent = touchEvent;
        //state = PRE_ATTACK;
    } else if (touchEvent == tEVENT_RELEASE) {
        //currentEvent = touchEvent;
        for (int k = 0; k < strip.numPixels(); k++) {
            strip.setPixelColor(k, 0, 0, 0);
        }
        strip.show();
    }

    /*if(touchEventCheck() returns touched) {
        touchSense();
    }
    delay(1000);*/
}

//------------------------------------------------------------
// touch event check
//
// check touch sensor for events:
//      tEVENT_NONE     no change
//      tEVENT_TOUCH    sensor is touched (Low to High)
//      tEVENT_RELEASE  sensor is released (High to Low)
//
//------------------------------------------------------------
int touchEventCheck() {
  int touchSense;                     // current reading
  static int touchSenseLast = LOW;    // last reading, set LOW first time

  static unsigned long touchDebounceTimeLast = 0; // debounce timer, set to 0 first time
  int touchDebounceTime = 50;                     // debounce time

  static int touchNow = LOW;  // current debounced state, set LOW first time
  static int touchLast = LOW; // last debounced state, set LOW first time

  int tEvent = tEVENT_NONE;   // default event

  // read touch sensor
  tReading = touchSampling();

  // touch sensor is HIGH if trigger point some threshold above Baseline
  if (tReading>(tBaseline + tBaseline/SENSITIVITY)) {
    touchSense = HIGH;
  } else {
    touchSense = LOW;
  }

  // debounce touch sensor
  // if state changed then reset debounce timer
  if (touchSense != touchSenseLast) {
    touchDebounceTimeLast = millis();
  }
  touchSenseLast = touchSense;

  // accept as a stable sensor reading if the debounce time is exceeded without reset
  if (millis() > touchDebounceTimeLast + touchDebounceTime) {
    touchNow = touchSense;
  }

  // set events based on transitions between readings
  //LOW to HIGH
  if (!touchLast && touchNow) {
    tEvent = tEVENT_TOUCH;
  }
  //HIGH to LOW
  if (touchLast && !touchNow) {
    tEvent = tEVENT_RELEASE;
  }

  // update last reading
  touchLast = touchNow;
  return tEvent;
}

void touchSense() {
  tR = micros(); //set a sense at the current time
}

void touchSensejerad() { //FN WORKING
    i+=20;
    while (digitalRead(rPin) == HIGH) {
        i%=256;
        color = wheelColor(i,255);
        for (int j = 0; j < strip.numPixels(); j+=2) {
            strip.setPixelColor(j, color);
        }
        strip.show();
        i++;
        
        delay(10);
    }
    //Convert the int to a string
    char tmpStr[64];
    sprintf(tmpStr, "%lu", color);  //assuming fahrenheit is a float variable
    Particle.publish("FamiLamp", tmpStr);
}

//------------------------------------------------------------
// touch sampling
//
// sample touch sensor SAMPLE_SIZE times and get average RC delay [usec]
//------------------------------------------------------------
long touchSampling() {
  long tDelay = 0;
  int mSample = 0;

  for (int i=0; i<SAMPLE_SIZE; i++) {
    pinMode(rPin, OUTPUT); // discharge capacitance at rPin
    digitalWrite(sPin, LOW);
    digitalWrite(rPin, LOW);
    pinMode(rPin, INPUT); // revert to high impedance input
    // timestamp & transition sPin to HIGH and wait for interrupt in a read loop
    tS = micros();
    tR = tS;
    digitalWrite(sPin, HIGH);
    do {
      // wait for transition
    } while (digitalRead(rPin)==LOW);

    // accumulate the RC delay samples
    // ignore readings when micros() overflows
    if (tR>tS) {
      tDelay = tDelay + (tR - tS);
      mSample++;
    }
  }

  // calculate average RC delay [usec]
  if ((tDelay > 0) && (mSample>0)) {
    tDelay = tDelay/mSample;
  } else {
    tDelay = 0;     // this is an error condition!
  }
  
  //autocalibration using exponential moving average on data below specified point
  if (tDelay<(tBaseline + tBaseline/BASELINE_SENSITIVITY)) {
    tBaseline = tBaseline + (tDelay - tBaseline)/BASELINE_VARIANCE;
  }
  return tDelay;
}

void commReceived(const char *event, const char *data)
{
    Particle.publish("debug","commReceived");
    uint32_t color1 = strtoul(data, NULL, 0);
    for (int j = 1; j < strip.numPixels(); j+=2) {
        strip.setPixelColor(j, color);
    }
    strip.show();
}

//------------------------------------------------------------
// Wheel
//------------------------------------------------------------

uint32_t wheelColor(byte WheelPos, byte iBrightness) { //FN WORKING
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
