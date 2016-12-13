# firefly-snowflake
Firefly Design LLC - Snowflake Christmas Ornament

The snowflake is a Christmas tree ornament with RGB LEDs that light up in various ways.  It has a rechargeable battery (via micro USB port) and it is controllable via BLE via a Swift app.

This repository has all the files to create a snowflake Christmas ornament.  This includes 3D mechanical files to create the plastic parts, Eagle schematics & layout for the PCBA, PCBA firmware, and Swift App source files.

To compile the firmware you need the Nordic nRF5 SDK 12 and Firefly Ice Firmware.  On macOS:

    $ git clone https://github.com/denisbohm/firefly-snowflake.git
    $ cd firefly-snowflake/firmware
    $ git clone https://github.com/denisbohm/firefly-ice-firmware.git
    $ curl https://www.nordicsemi.com/eng/nordic/download_resource/54291/51/54256290 -o nRF5_SDK_12.zip
    $ unzip nRF5_SDK_12.zip -d nRF5_SDK_12

See [Firefly Design LLC](http://fireflydesign.com) for information on the open source Firefly Design repositories.

## Copyright and License
Copyright 2016 Firefly Design LLC / Denis Bohm

![License](http://i.creativecommons.org/l/by/4.0/88x31.png)

This work is licensed under [Creative Commons Attribution 4.0 International](https://licensebuttons.net/l/by/4.0/88x31.png).
