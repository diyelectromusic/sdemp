# Pi Pico MIDI H(Arp)
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
import ustruct
import picowireless
from time import sleep

led  = machine.Pin(25, machine.Pin.OUT)
uart = machine.UART(0,31250)

ssid=[]
channel=[]
rssi=[]

picowireless.init()
numnets = 0

MIDICH=1
playidx=[]
timecnt=[]
scancounter=0

midiseq=[]
midilen=[]
midivol=[]

def scanNetworks():
    global ssid, channel, rssi, playidx, numnets
    ssid=[]
    channel=[]
    rssi=[]
    picowireless.start_scan_networks()
    numnets = picowireless.get_scan_networks()
    for net in range (numnets):
        # We get a weird "String\x00\x00\x00\x00" type response - i.e. it is padded with NULLs
        # which causes issues when trying to print it out, so split on the first NULL to create
        # a proper python string.
        ssid.append(picowireless.get_ssid_networks(net).split('\x00',1)[0])
        channel.append(picowireless.get_channel_networks(net))
        rssi.append(picowireless.get_rssi_networks(net))

def printNetworks():
    global ssid, channel, rssi, numnets
    print('{0:18s}'.format("SSID"), '{0:4s}'.format("CH"), '{0:6s}'.format("RSSI"))
    for net in range(numnets):
        print('{0:18s}'.format(ssid[net]), '{0:4d}'.format(channel[net]), '{0:6d}'.format(rssi[net]))

def wifi2midi():
    global midiseq, midilen, midivol, playidx, timecnt
    midiseq=[]
    midilen=[]
    midivol=[]
    playidx=[]
    timecnt=[]
    for net in range (numnets):
        midilen.append(channel[net])
        midivol.append(127+rssi[net])
        midiseq.append(bytearray(ssid[net]))
        for c in range (len(midiseq[net])):
            midiseq[net][c] = ssid2Midi(midiseq[net][c])
        playidx.append(0)
        timecnt.append(0)

# This takes an ASCII character from the SSID and returns the MIDI note number to use
# MIDI Starts at C1; ASCII starts at 32 (space)
def ssid2Midi (c):
    if (c<32 or c>126):
        return 0
    return 24 + c - 32

def midiNoteOn (note, vel):
    #print ("NoteOn:", note, " (", vel, ")")
    uart.write(ustruct.pack("bbb",0x90+MIDICH-1,note,vel))

def midiNoteOff (note):
    #print ("NoteOff:", note)
    uart.write(ustruct.pack("bbb",0x80+MIDICH-1,note,0))

scanNetworks()
printNetworks()
wifi2midi()

print (midilen)
print (midivol)
for net in range (numnets):
    print("".join("%d " %i for i in midiseq[net])," [",len(midiseq[net]),"]")

while(1):
    # Every scan, update and check the timer counters and act accordingly
    led.value(1)
    print ("--------------------------------------------")
    print ("len ",midilen)
    print ("cnt ",timecnt)
    print ("idx ",playidx)
    for i in range (numnets):
        timecnt[i] += 1
        if (timecnt[i] >= midilen[i]):
            timecnt[i] = 0
            midiNoteOff(midiseq[i][playidx[i]])
            playidx[i] += 1
            if (playidx[i] >= len(midiseq[i])):
                playidx[i] = 0
            midiNoteOn(midiseq[i][playidx[i]], midivol[i])

    led.value(0)

    scancounter += 1
    if (scancounter >= 60):
        scancounter = 0
        print ("[----- Scanning Networks -----]")
        scanNetworks()
        print ("[----- Found ", numnets, " Networks -----]")
        #printNetworks()
        wifi2midi()
        print ("[----- Converted SSID/CH/RSSI to MIDI -----]")

    sleep(1)
