# RP2350 ZX Spectrum Compatible Video for RC2014

This is a version of the Raspberry Pi provided [scanvideo library](https://github.com/raspberrypi/pico-extras/tree/master/src/common/pico_scanvideo) with adjustments made to allow it to run under the unofficial Ardiuno RP2040/RP2350 core and to support the RGBY1111 CGA-like mode.  This provides ZX Spectrum compatible video graphics for RC2014 and siimilar systems.

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

## tap2basic.py

This is a short python script that will read in a ZX Spectrum TAP file and pull out the datablock it suspects is the loading screen and then output to the console a BASIC program that can be copied over to an RC2014 to display the screen.

The name of the TAP file is hardcoded at the top of the file.

# License

All information is provided AS IS with no implied fit for purpose as detailed in the included MIT License.
This code MUST NOT be used for thr training of AI systems.

All content and code (c) emalliab.wordpress.com (Kevin)
