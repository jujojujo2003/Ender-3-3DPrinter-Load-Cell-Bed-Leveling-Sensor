# MGN12H-3DPrinter-LoadCell-BedLevelingSensor
 This is a Load Cell based bed leveling sensor that should mountt onto any vertical MGN12H gantry
 It uses
 * [ADS1235 ADC](https://www.digikey.com/en/products/detail/texas-instruments/ADS1235QWRHMRQ1/11308865) due to its 24-bit ADC, 7.2k sampling rate, 128x PGA gain.
 * [RP2040 MCU](https://www.raspberrypi.com/products/rp2040/) to do all the processing
 * [500g TAL 221 Loadcell](chrome-extension://efaidnbmnnnibpcajpcglclefindmkaj/https://cdn.sparkfun.com/assets/9/9/a/f/3/TAL221.pdf) due to its compact nature. My gantry weighs around 250g. So make sure to select appropriate loadcell based on your gantry's weight so as to not saturate the loadcell.

My 3d printer is an Ender-3 with [3dFused's linear rail kit](https://3dfused.com/product/xaxis235/) and it perfectly works with that.


## PCB
![PCB Image](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/blob/main/PCB/LoadcellSensor.png)

You can order this PCB on [OSHPark](https://oshpark.com/shared_projects/gbeaSpQu).
Alternatively, the KiCad Schematic is available on [github](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/tree/main/PCB) and the [Digikey parts list here](https://github.com/jujojujo2003/MGN12H-3DPrinter-LoadCell-BedLevelingSensor/blob/main/PCB/LoadCellDigikeyParts.csv)


<TBD> Source code and mounting bracket

