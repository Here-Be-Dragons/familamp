# familamp
###### An IoT lamp based on John Harrison's Filimin networked RGB lights.

![Alt text](/images/Lamp_example.jpg?raw=true "Shameless better-than-actual example")

### Overview
---------------------
Networked lamps using a Particle Photon, WB2812B LEDs, and custom touch-sensitive paneling.  They communicate and syncronize colors across lamps via the internet and Particle's butt using Particle.subscribe() and Particle.publish() functions.   This can also be accompished via an Arduino + wifi chip, but I wanted to be able to push updates directly to lamps or troubleshoot them remotely with ease.  Additionally, I could theoretically add features and push updates for that as well.

John Harrrison's Filimin: 
https://www.hackster.io/filimin/networked-rgb-wi-fi-decorative-touch-lights

### Hardware
---------------------
-   Particle Photon
-   WB2812B strip of LEDs.  I used 30 per lamp.
-   A sheet of acrylic
-   Wood for the base and any flourishes you want for your lamps
-   Custom SMD PCB (to be designed on oshpark.com)

### Software
---------------------
-   Particle's Butt
-   NeoPixel Library
