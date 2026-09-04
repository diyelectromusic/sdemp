# RP2350 ZX Spectrum Compatible Video for RC2014

This is a version of the Raspberry Pi provided [scanvideo library](https://github.com/raspberrypi/pico-extras/tree/master/src/common/pico_scanvideo) with adjustments made to allow it to run under the unofficial Ardiuno RP2040/RP2350 core and to support the RGBY1111 and RGB222 CGA-like modes.  This provides ZX Spectrum compatible video graphics for RC2014 and siimilar systems.

Much of the ZX Spectrum display handling code is based on the code from the [pico-zxspectrum project](https://github.com/fruit-bat/pico-zxspectrum).

Full details see: https://emalliab.wordpress.com/2026/08/03/zx-spectrum-compatible-video-for-rc2014/

## Instructions for reproducing the sketch

Copy the contents of this folder to a new sketch. PicoRC2014Video.ino is the main file.  Then copy the following files from the Raspberry Pi pico-extras gitHub into the sketch:

```
src/common/pico_scanvideo/vga_modes.c
src/common/pico_scanvideo/include/pico/scanvideo/composable_scanline.h
src/common/pico_scanvideo/include/pico/scanvideo/scanvideo_base.h
src/rp2_common/pico_scanvideo_dpi/scanvideo.c
src/rp2_common/pico_scanvideo_dpi/include/pico/scanvideo.h
src/common/pico_util_buffer/buffer.c
src/common/pico_util_buffer
```

Edit all these files to remove any directory structure in the #include statements, to collapse references to the above files into the same directory.

Add the following to the top of scanvideo.h:
```
#include "vgamode.h"
```

Now take the two PIO programs and assemble them to C source.  I used https://wokwi.com/tools/pioasm
```
src/common/pico_scanvideo/scanvideo.pio -> scanvideo-pio.h
src/rp2_common/pico_scanvideo_dpi/timing.pio -> timing-pio.h
```

Finally, perrform the changes shown in the scanvideo.c.diff file to scanvideo.c and remove the diff file, readme and Python script from the sketch folder.

This should now build as a sketch within Earle F. Philhower's Arduino Pico core: https://github.com/earlephilhower/arduino-pico

## Configuration Options

Please refer back to the blog series for full details of the configuration options and use, but a summary is provided below.

The file vgamode.h has an option to build for RGBY1111 or RGB222 video modes.  By default is it set up for RGB222.

It is also configured to use the higher PIO GPIO base (16-47), assuming the video pins start at GPIO 40.

The file PicoRC2014Video has Z80_INT_50HZ enabled which will generate a 50Hz interrupt signal on GPIO 33.  The GPIO can be changed or the signal can be disabled by commenting out the Z80_INT_50HZ define.

*Please Note*: On V1 of the PCB GPIO 33 is directly tied to /INT, so won''t really work.  On V2 of the PCB it goes through a 74HCT14 inverter to create a clean /INT signal at 50Hz.

## Release for RC2014 Spectrum Video V2

A UF2 file for use with the Pimoroni PGA2350 and V2 of my PCB is provided, but this is **use at your own risk**.

Full details can be found here: 

## tap2basic.py

This is a short python script that will read in a ZX Spectrum TAP file and pull out the datablock it suspects is the loading screen and then output to the console a BASIC program that can be copied over to an RC2014 to display the screen.

The name of the TAP file is hardcoded at the top of the file.

# License

All information is provided AS IS with no implied fit for purpose as detailed in the included MIT License.
This code MUST NOT be used for thr training of AI systems.

All content and code (c) emalliab.wordpress.com (Kevin)
