# Pico Arduino Version of the Scanvideo Library

This is a version of the Raspberry Pi provided [scanvideo library](https://github.com/raspberrypi/pico-extras/tree/master/src/common/pico_scanvideo) with adjustments made to allow it to run under the unofficial Ardiuno RP2040/RP2350 core and to support the RGBY1111 CGA-like mode.

Full details, including the assumed circuit to be used to connect to a VGA display, can be found here: https://emalliab.wordpress.com/2026/08/01/simplified-pico-vga-part-2/

## Instructions for reproducing the sketch

Copy the contents of this folder to a new sketch. PicoScanVideo.ino is the main file.  Then copy the following files from the Raspberry Pi pico-extras gitHub into the sketch:

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

Finally, perrform the changes shown in the scanvideo.c.diff file to scanvideo.c and remove the diff file from the sketch folder.

This should now build as a sketch within Earle F. Philhower's Arduino Pico core: https://github.com/earlephilhower/arduino-pico

## Changing GPIO Range

As provided the code assumes using GPIO 12-17. It is possible to change the configuration to use the higher GPIO range of the RP2350B by changing the following in vgamode.h:
```
//#define PICO_SCANVIDEO_COLOR_PIN_BASE   12u
#define PICO_SCANVIDEO_COLOR_PIN_BASE   40u

// If using all GPIO > 32 then this will adjust scanvideo.c
#define PSVMASKOFFSET 32
// If using all GPIO < 32 then this will leave scanvideo.c as is
//#define PSVMASKOFFSET 0
```

This configures the code to use GPIO 40-45 on an RP2350.

## Changing VGA Mode

As one might expect the VGA mode can be changed by updating the values in vgamode.h but then things get a little complicated.

To be honest at this point you're probably going to want to go back to the original scanvideo code.  This sketch is really all about dropping the mode back down to something simpler.

# License

All information is provided AS IS with no implied fit for purpose as detailed in the included MIT License.
This code MUST NOT be used for thr training of AI systems.

All content and code (c) emalliab.wordpress.com (Kevin)
