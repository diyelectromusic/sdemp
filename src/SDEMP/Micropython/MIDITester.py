# MIDI Channel Tester
# for Micro Python on the Raspberry Pi Pico
#
# @diyelectromusic
# https://diyelectromusic.wordpress.com/
#
#      MIT License
#      
#      Copyright (c) 2020 diyelectromusic (Kevin)
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
import machine
import utime
import ustruct
import random

CH = 1

pin = machine.Pin(25, machine.Pin.OUT)
uart1 = machine.UART(0,31250)
uart2 = machine.UART(1,31250)

# C3 D3 E3 F3 G3 A3 B3   C4 D4 E4 F4 G4 A4 B4   C5 D5 E5 F5 G5 A5 B5 C6
# 48 50 52 53 55 57 59   60 62 64 65 67 69 71   72 74 76 77 79 81 83 84
notes = [
   48, 50, 52, 53, 55, 57, 59, 60,
]

tempo = random.randint(20,1000)
diff  = random.randint(100,500)

while True:
    for x in notes:
        pin.value(1)
        uart1.write(ustruct.pack("bbb",0x90|(CH-1),x,127))
        utime.sleep_ms(diff)
        uart2.write(ustruct.pack("bbb",0x90|(CH),x+12,127))
        utime.sleep_ms(120)
        pin.value(0)
        utime.sleep_ms(tempo)
        uart1.write(ustruct.pack("bbb",0x80|(CH-1),x,0))
        utime.sleep_ms(diff)
        uart2.write(ustruct.pack("bbb",0x80|(CH),x+12,0))
