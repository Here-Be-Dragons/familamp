# familamp
###### An IoT lamp based on John Harrison's Filimin and myk3y's familamp networked RGB lights.

### Overview
---------------------
Networked lamps using a Particle Photon, WB2812B LEDs, and custom touch-sensitive paneling.  They communicate and syncronize colors across lamps via the internet and Particle's cloud using Particle.subscribe() and Particle.publish() functions.   This can also be accompished via an Arduino + wifi chip, but I wanted to be able to push updates directly to lamps or troubleshoot them remotely with ease.  Additionally, I could theoretically add features and push updates for that as well.

John Harrrison's Filimin: 
https://www.hackster.io/filimin/networked-rgb-wi-fi-decorative-touch-lights


### Hardware
---------------------
-   Particle Photon
-   WB2812B strip of LEDs.  I used 30 per lamp.
-   A sheet of acrylic
-   Wood for the base and any flourishes you want for your lamps
-   Custom SMD PCB: https://oshpark.com/shared_projects/9PPIsyNF - my first SMD board - I hope it works!

### Software
---------------------
Particle's Cloud
NeoPixel Library
