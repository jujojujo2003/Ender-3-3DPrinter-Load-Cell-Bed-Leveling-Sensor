# MGN12H-3DPrinter-LoadCell-BedLevelingSensor
 This is a Load Cell based bed leveling sensor that should mountt onto any vertical MGN12H gantry
 It uses
 * [ADS1235 ADC](https://www.digikey.com/en/products/detail/texas-instruments/ADS1235QWRHMRQ1/11308865) due to its 24-bit ADC, 7.2k sampling rate, 128x PGA gain.
 * [RP2040 MCU](https://www.raspberrypi.com/products/rp2040/) to do all the processing
 * [500g TAL 221 Loadcell](https://cdn.sparkfun.com/assets/9/9/a/f/3/TAL221.pdf) due to its compact nature. My gantry weighs around 250g. So make sure to select appropriate loadcell based on your gantry's weight so as to not saturate the loadcell.

My 3d printer is an Ender-3 with [3dFused's linear rail kit](https://3dfused.com/product/xaxis235/) and it perfectly works with that.


## PCB
![PCB Image](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/blob/main/PCB/LoadcellSensor.png)

You can order this PCB on [OSHPark](https://oshpark.com/shared_projects/gbeaSpQu).
Alternatively, the KiCad Schematic is available on [github](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/tree/main/PCB) and the [Digikey parts list here](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/blob/main/PCB/LoadCellDigikeyParts.csv)


<TBD> Source code and mounting bracket

## Code

The Arduino IDE code is available [here](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/tree/main/Code). 
* Install [Arduino IDE](https://www.arduino.cc/en/software)
* Install the following Arduino Libraries
  * [PioSPI](https://www.arduino.cc/reference/en/libraries/piospi/)
  * [Adafruit NeoPixel](https://www.arduino.cc/reference/en/libraries/adafruit-neopixel/)
  * [RunningAverage](https://www.arduino.cc/reference/en/libraries/runningaverage/)
* Try compiling the binary

Alternatively, you can flash the [precompiled binary](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/releases/tag/release)

## Normal Operation
![Assembled Sensor](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/blob/main/WikiAssets/IMG_0561.jpg)

If everything is wired correctly, as soon as you power on, the LED should we RED for a few seconds and then turn off. If it flashes red, then there is an error. Try connecting the USB port and check the debug message printed to serial. 

![Initialization](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/blob/main/WikiAssets/IMG_0559.gif)

When the sensor is probing, the LED should turn blue to indicate that it is probing and turn green for a short while when triggered.
![Triggered](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/blob/main/WikiAssets/IMG_0551.gif)

## Tuning
[DELTA_FOR_TRIGGER](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/blob/main/Code/bedlevelRp2040ADS1235.ino#L8) is the main tuning variable that controls the sensitivity of the trigger.

* Typically, set DELTA_FOR_TRIGGER to a low value like 1-2 and set `is_probing = true`
* Reset your printer and as soon as it starts, the LED should be BLUE. Note that it might get triggers very easily and turn green since the threshold above is low.
* Keep the filament wire into the extruder tight and try to extrude filament or move the printer head and keep bumping up DELTA_FOR_TRIGGER till such motions don't trigger.
* Now the probe for probing motions
