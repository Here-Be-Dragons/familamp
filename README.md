# familamp

![Alt text](/images/finished_lamp.jpg?raw=true "Finished Lamp Example")

### Overview
---------------------
Networked lamps using a Particle Photon, WB2812B LEDs (aka NeoPixels), and a touch-sensitive panel.  They communicate and synchronize colors across lamps via the internet and Particle's butt using Particle.subscribe() and Particle.publish() functions.  Bonus of using Particle's cloud: easily push updates and add features from Particle's browser-based IDE and Console.

### Images
---------------------
Demo of operation:  
![Alt text](/images/Demo.gif?raw=true "Demo Operation")  
https://youtu.be/uUX5NEsEXrw
  
Special mode idle sequences (instead of being off, the lamp runs an assigned idle sequence on holidays)
![Alt text](/images/idle1.gif?raw=true "Christmas Idle")
![Alt text](/images/idle2.gif?raw=true "New Years/4th of July")
![Alt text](/images/idle3.gif?raw=true "Birthdays")
  
![Alt text](/images/leds-bottom.jpg?raw=true "Bottom of lamp")
  
Circuit boards from Osh Park:
![Alt text](/images/osh_boards.jpg?raw=true "Osh Park boards")

### Hardware
---------------------
-   Particle Photon
-   WB2812B strip of LEDs.  I used 60 per lamp, because this seemed like a good match for my 2A power supplies.
-   A 3.3V to 5V logic level shifter to convert the 3.3V data bus from the Particle Photon to 5V the NeoPixels require.
-   Pre-cut acrylic pieces to make the acrylic light box.  TAP Plastics was the vendor I chose for this. (See *Other Note #1*)  For each lamp:
    -   2 cut-to-size 1/8" thick pieces, 5" x 9-7/8"
	-   2 cut-to-size 1/8" thick pieces, 4-3/4" x 9-7/8"
	-   1 cut-to-size 1/8" thick piece, 5" x 5"
-   A sheet of thin copper for the top.  I used a smooth ball peen hammer on the copper before installation to give it a hammered look.
-   Wood for the base and any flourishes you want for your lamps
-   3/4" x 3/4" outside corner moulding (if desired, to hide seems).
    -   http://www.homedepot.com/p/Alexandria-Moulding-WM-206-3-4-in-x-3-4-in-x-96-in-Wood-Red-Oak-Outside-Corner-Moulding-0W206-40096C/205917014
-   Custom PCB (I designed this in EAGLE and requested manufacture by OSH Park)
-   Other supporting bits (per lamp):
	-   1 resistor of value between 300 and 500 Ohms
	-   1 resistor of value between 1MOhm and 10MOhm (See *Other Note #2*)
	-   A barrel jack of matching dimensions to your power supply (Mine were 5.5mm with a 2.1mm center pole)
	-   A 5V power supply able to power the lamp.  I chose a UL listed supply of 2A. (See *Other Note #3*)
	-   Feet for the bottom of the lamp.  I grabbed some rubber feet from my local hardware supply store.
	-   Mounting hardware to attach the PCB.  I used small screws and standoffs from my local hardware supply store.

### Software
---------------------
-   Particle's Cloud
-   NeoPixel Library: https://github.com/adafruit/Adafruit_NeoPixel (Already ported to Particle's Libraries)
-   CapSense library: https://github.com/lbt/captouch (Already ported to Particle's Libraries)

### Assembly
---------------------
-   1MOhm resistor between pins D3 and D4 of the Particle Photon
-   D2 goes to a 3.3V to 5V Logic Level Shifter, then to a resistor of value between 300 to 500 Ohms, then to the data input pin of the NeoPixels.
    -   The resistor prevents voltage spikes that might otherwise damange the first pixel in the string.
-   D3 of the Particle Photon is connected to the touch sensitive hammered copper panel on the top of the lamp.
-   Connect a capacitor (1000µF, 6.3V or higher) across VIN and GND on the Particle Photon, then connect VIN and GND to the corresponding NeoPixel pins.
    -   The 1000µF capacitor prevents the initial onrush of current from damaging the pixels over time.  I ended up using a 500µF capacitor instead so it would fit my design's physical contraints better.
-   I used a 9" length of 1" PVC pipe to wrap the NeoPixels around, and hot-glued this into a hole in the top of the wood base.
    -   Do not use hot glue to glue the NeoPixels to the PVC pipe.  Either buy the NeoPixels with 3M tape on the back or use something like silicone (do not power the pixels on until after the time indicated for complete dry time on the glue of choice to prevent shorting the pixels).  When I used hot glue, this caused the strip to stop working correctly.

### Other Notes & Lessons Learned
---------------------
1.  I tried saving money here by buying a large sheet of clear acrylic and cutting it myself, then sanding to make clear into frosted.  It turns out cutting acrylic is not easy to do well.  Abandoned this idea and bought clear acrylic pre-cut from a vendor, then sanded with 60-grit using a random orbital sander to frost the acrylic.  This still took forever.  If I were to do it again, I'd just buy pre-frosted with a medium light pass-through level.
2.  A lower resistance is less sensitive, but also polls faster. A high resistance takes longer to poll but can detect changes to capacitance with more sensitivity.  Prototype between 1MOhm and 10MOhm to see what works with your design.  I chose 1m ohm because the code loops back through a the captouch poll function frequently in the middle of effect transitions. It does this to try to be responsive to touch as quickly as possible.  In really dry weather it does slow down polling significantly which has the end result of making slightly stuttery animation effects, even with 1MOhm.
3.  Try to avoid power supplies that are not UL-Listed.  2A was a good middle ground for a reliable supply.  This device draws ~1A at 5V on its brightest setting during typical usage.  If all LEDs are at maximum brightness white (not something available in the current code), the lamp draws almost 3A.