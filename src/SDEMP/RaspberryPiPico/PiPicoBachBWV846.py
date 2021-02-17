# J.S.Bach Prelude in C Major BWV846
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

pin = machine.Pin(25, machine.Pin.OUT)
uart = machine.UART(1,31250)

# C3 D3 E3 F3 G3 A3 B3   C4 D4 E4 F4 G4 A4 B4   C5 D5 E5 F5 G5 A5 B5 C6
# 48 50 52 53 55 57 59   60 62 64 65 67 69 71   72 74 76 77 79 81 83 84
notes = [
    60, 64, 67, 72, 76, 67, 72, 76,   60, 64, 67, 72, 76, 67, 72, 76,
    60, 62, 69, 74, 77, 69, 74, 77,   60, 62, 69, 74, 77, 69, 74, 77,
    59, 62, 67, 74, 77, 67, 74, 77,   59, 62, 67, 74, 77, 67, 74, 77,
    60, 64, 67, 72, 76, 67, 72, 76,   60, 64, 67, 72, 76, 67, 72, 76,
    60, 64, 69, 76, 81, 69, 76, 81,   60, 64, 69, 76, 81, 69, 76, 81,
    60, 62, 66, 69, 74, 66, 69, 74,   60, 62, 66, 69, 74, 66, 69, 74,
    59, 62, 67, 74, 79, 67, 74, 79,   59, 62, 67, 74, 79, 67, 74, 79,
    59, 60, 64, 67, 72, 64, 67, 72,   59, 60, 64, 67, 72, 64, 67, 72,
    57, 60, 64, 67, 72, 64, 67, 72,   57, 60, 64, 67, 72, 64, 67, 72,
    50, 57, 62, 66, 72, 62, 66, 72,   50, 57, 62, 66, 72, 62, 66, 72,
    55, 59, 62, 67, 71, 62, 67, 71,   55, 59, 62, 67, 71, 62, 67, 71,
    55, 58, 64, 67, 73, 64, 67, 73,   55, 58, 64, 67, 73, 64, 67, 73,
    53, 57, 62, 69, 74, 62, 69, 74,   53, 57, 62, 69, 74, 62, 69, 74,
    53, 56, 62, 65, 71, 62, 65, 71,   53, 56, 62, 65, 71, 62, 65, 71,
    52, 55, 60, 67, 72, 60, 67, 72,   52, 55, 60, 67, 72, 60, 67, 72,
    52, 53, 57, 60, 65, 57, 60, 65,   52, 53, 57, 60, 65, 57, 60, 65,
    50, 53, 57, 60, 65, 57, 60, 65,   50, 53, 57, 60, 65, 57, 60, 65,
    43, 50, 55, 59, 65, 55, 59, 65,   43, 50, 55, 59, 65, 55, 59, 65,
    48, 52, 55, 60, 64, 55, 60, 64,   48, 52, 55, 60, 64, 55, 60, 64,
    48, 55, 58, 60, 64, 58, 60, 64,   48, 55, 58, 60, 64, 58, 60, 64,
    41, 53, 57, 60, 64, 57, 60, 64,   41, 53, 57, 60, 64, 57, 60, 64,
    42, 48, 57, 60, 63, 57, 60, 63,   42, 48, 57, 60, 63, 57, 60, 63,
    44, 53, 59, 60, 62, 59, 60, 62,   44, 53, 59, 60, 62, 59, 60, 62,
    43, 53, 55, 59, 62, 55, 59, 62,   43, 53, 55, 59, 62, 55, 59, 62,
    43, 52, 55, 60, 64, 55, 60, 64,   43, 52, 55, 60, 64, 55, 60, 64,
    43, 50, 55, 60, 65, 55, 60, 65,   43, 50, 55, 60, 65, 55, 60, 65,
    43, 50, 55, 59, 65, 55, 59, 65,   43, 50, 55, 59, 65, 55, 59, 65,
    43, 51, 57, 60, 66, 57, 60, 66,   43, 51, 57, 60, 66, 57, 60, 66,
    43, 52, 55, 60, 67, 55, 60, 67,   43, 52, 55, 60, 67, 55, 60, 67,
    43, 50, 55, 60, 65, 55, 60, 65,   43, 50, 55, 60, 65, 55, 60, 65,
    43, 50, 55, 59, 65, 55, 59, 65,   43, 50, 55, 59, 65, 55, 59, 65,
    36, 48, 55, 58, 64, 55, 58, 64,   36, 48, 55, 58, 64, 55, 58, 64,
    36, 48, 53, 57, 60, 65, 60, 57,   60, 57, 53, 57, 53, 50, 53, 50,
    36, 47, 67, 71, 74, 77, 74, 71,   74, 71, 67, 71, 62, 65, 64, 62,
]

for x in notes:
    pin.value(1)
    uart.write(ustruct.pack("bbb",0x90,x,127))
    utime.sleep_ms(120)
    pin.value(0)
    utime.sleep_ms(50)
    uart.write(ustruct.pack("bbb",0x80,x,0))

pin.value(1)
uart.write(ustruct.pack("bbb",0x90,36,127))
uart.write(ustruct.pack("bbb",0x90,48,127))
uart.write(ustruct.pack("bbb",0x90,64,127))
uart.write(ustruct.pack("bbb",0x90,67,127))
uart.write(ustruct.pack("bbb",0x90,72,127))
utime.sleep_ms(2000)
pin.value(0)
uart.write(ustruct.pack("bbb",0x80,36,127))
uart.write(ustruct.pack("bbb",0x80,48,127))
uart.write(ustruct.pack("bbb",0x80,64,127))
uart.write(ustruct.pack("bbb",0x80,67,127))
uart.write(ustruct.pack("bbb",0x80,72,127))
