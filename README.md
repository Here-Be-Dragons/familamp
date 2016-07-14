# familamp
###### An IoT lamp based on John Harrison's Filimin networked RGB lights.

![Alt text](/images/SketchUp/familamp_3d.jpg?raw=true "Shameless better-than-actual example")

### Overview
---------------------
Networked lamps using a Particle Photon, WB2812B LEDs, and custom touch-sensitive paneling.  They communicate and syncronize colors across lamps via the internet and Particle's butt using Particle.subscribe() and Particle.publish() functions.   This can also be accompished via an Arduino + wifi chip, but I wanted to be able to push updates directly to lamps or troubleshoot them remotely with ease.  Additionally, I could theoretically add features and push updates for that as well.

### Hardware
---------------------
-   Particle Photon
-   WB2812B strip of LEDs.  I used 30 per lamp.
-   A sheet of acrylic, no thicker than 1/8".
-   Wood for the base and any flourishes you want for your lamps
-   Custom SMD PCB (to be designed on oshpark.com)

### Software
---------------------
-   Particle's Butt
-   NeoPixel Library
-   CapacitiveSensor library: https://github.com/PaulStoffregen/CapacitiveSensor

### Assembly
---------------------
-   10MOhm resistor between pins D3 and D4 of the Particle Photon
-   D2 goes to a 3.3V to 5V Logic Level Shifter, then to a resistor of value between 300 to 500 Ohms, then to the data input pin of the NeoPixels.
    -   The resistor prevents voltage spikes that might otherwise damange the first pixel in the string.
-   D3 of the Particle Photon is connected to the touch sensitive paint on the 4 panes.
-   Connect a capacitor (1000 µF, 6.3V or higher) across VIN and GND on the Particle Photon, then connect VIN and GND to the corresponding NeoPixel pins.
    -   The 1000 µF capacitor prevents the initial onrush of current from damaging the pixels over time.

### Debug:
---------------------
To help you with any problems, there are two booleans you can set at the beginning of the code: #D_SERIAL true will output debugging values to the serial port.#D_WIFI true will output debugging values to the Spark Butt.
The two values you want to note when debugging touch sensing are tBaseline and tDelay. tBaseline is an averaged floating point value calculated as a timed rate of decay for the touch sensor when nobody is touching it. tDelay is a more current value which is compared to tBaseline to detect touch. tBaseline will typically hover between 100 and 250ish. If it is higher, you probably have a bad connection. If it is lower, that suggests a short or lower quality power supply.

**Reading Debugging Values from the Spark Butt:**

When #D_WIFI is set to true you can see the values for tBaseline and tDelay in Linux using the Spark CLI:
```
watch -n 0.5 "curl -s -G https://api.particle.io/v1/devices/<device_id>/tBaseline -d access_token=<access token from Particle> | grep result"
watch -n 0.5 "curl -s -G https://api.particle.io/v1/devices/<device_id>/tDelay -d access_token=<access token from Particle> | grep result"
```
