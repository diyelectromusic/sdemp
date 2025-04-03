# Krell Display for EuroRack
#
# @diyelectromusic
# https://diyelectromusic.wordpress.com/
#
#      MIT License
#      
#      Copyright (c) 2025 diyelectromusic (Kevin)
#      
#      Permission is hereby granted, free of charge, to any person obtaining a copy of
#      this software and associated documentation files (the "Software"), to deal in
#      the Software without restriction, including without limitation the rights to
#      use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
#      the Software, and to permit persons to whom the Software is furnished to do so,
#      subject to the following conditions:
#      
#      The above copyright notice and this permission notice shall be included in all
#      copies or substantial portions of the Software.
#      
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
#      FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
#      COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHERIN
#      AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
#      WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
import time
import board
import neopixel
from analogio import AnalogIn

cv_in = AnalogIn(board.A3)

pixel_pin1 = board.GP2
pixel_pin2 = board.GP3
num_pixels = 5

pixels1 = neopixel.NeoPixel(pixel_pin1, num_pixels, brightness=0.3, auto_write=False, pixel_order=neopixel.RGB)
pixels2 = neopixel.NeoPixel(pixel_pin2, num_pixels, brightness=0.3, auto_write=False, pixel_order=neopixel.RGB)

col = (80, 35, 0)

lastcv = -1
while True:
    cv = cv_in.value / 256

    if (lastcv != cv):
        lastcv = cv
        led = cv / 25
        for pix in range(5):
            if (pix < led and cv > 5):
                pixels1[pix] = col
            else:
                pixels1[pix] = 0
                
            if (pix+5 < led and cv > 5):
                pixels2[pix] = col
            else:
                pixels2[pix] = 0
            
            pixels1.show()
            pixels2.show()
