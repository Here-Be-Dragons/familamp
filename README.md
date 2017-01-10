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
-   WB2812B strip of LEDs.  I used 60 per lamp
-   A sheet of acrylic, I used 1/8"
-   A sheet of thin copper for the top
-   Wood for the base and any flourishes you want for your lamps
-   3/4" x 3/4" outside corner moulding (if desired, to hide acrylic seems)
    -   http://www.homedepot.com/p/Alexandria-Moulding-WM-206-3-4-in-x-3-4-in-x-96-in-Wood-Red-Oak-Outside-Corner-Moulding-0W206-40096C/205917014
-   Custom PCB (I designed this in EAGLE and requested manufacture by OSH Park)

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
-   I used a length of 1" PVC to wrap the neopixels around, and hot-glued this into a hole in the top of the wood base.
