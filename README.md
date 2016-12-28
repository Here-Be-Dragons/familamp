# familamp

![Alt text](/images/SketchUp/familamp_3d.jpg?raw=true "Prototype picture placeholder")

### Overview
---------------------
Networked lamps using a Particle Photon, WB2812B LEDs, and a touch-sensitive panel.  They communicate and syncronize colors across lamps via the internet and Particle's butt using Particle.subscribe() and Particle.publish() functions.  Additionally, I am able to easily add features and deploy the new code from Particle's Console.

### Hardware
---------------------
-   Particle Photon
-   WB2812B strip of LEDs.  I used 30 per lamp
-   A sheet of acrylic, no thicker than 1/8"
-   A sheet of thin copper for the top
-   Wood for the base and any flourishes you want for your lamps
-   3/4" x 3/4" outside corner moulding (if desired, to hide acrylic seems)
    -   http://www.homedepot.com/p/Alexandria-Moulding-WM-206-3-4-in-x-3-4-in-x-96-in-Wood-Red-Oak-Outside-Corner-Moulding-0W206-40096C/205917014
-   Custom PCB (to be designed in EAGLE and manufactured by OSH Park)

### Software
---------------------
-   Particle's Butt
-   NeoPixel Library
-   CapSense library: https://github.com/lbt/captouch

### Assembly
---------------------
-   1MOhm resistor between pins D3 and D4 of the Particle Photon
-   D2 goes to a 3.3V to 5V Logic Level Shifter, then to a resistor of value between 300 to 500 Ohms, then to the data input pin of the NeoPixels.
    -   The resistor prevents voltage spikes that might otherwise damange the first pixel in the string.
-   D3 of the Particle Photon is connected to the touch sensitive hammered copper panel on the top of the lamp.
-   Connect a capacitor (1000 µF, 6.3V or higher) across VIN and GND on the Particle Photon, then connect VIN and GND to the corresponding NeoPixel pins.
    -   The 1000 µF capacitor prevents the initial onrush of current from damaging the pixels over time.
-   I used a length of 1" PVC to wrap the neopixels around.
