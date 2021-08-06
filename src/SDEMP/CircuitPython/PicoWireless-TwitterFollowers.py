# Pi Pico MIDI Twitter Followers
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
import board
import busio
import digitalio
import adafruit_requests as requests
import adafruit_esp32spi.adafruit_esp32spi_socket as socket
from adafruit_esp32spi import adafruit_esp32spi
import json
import time

led = digitalio.DigitalInOut(board.GP25)
led.direction = digitalio.Direction.OUTPUT
uart = busio.UART(board.GP0, board.GP1, baudrate=31250)

def noteOn (x):
    uart.write(bytes([0x90, x, 127]))

def noteOff (x):
    uart.write(bytes([0x80, x, 0]))

# Get wifi details and more from a secrets.py file
try:
    from secrets import secrets
except ImportError:
    print("WiFi secrets are kept in secrets.py, please add them there!")
    raise

esp32_cs = digitalio.DigitalInOut(board.GP7)
esp32_ready = digitalio.DigitalInOut(board.GP10)
esp32_reset = digitalio.DigitalInOut(board.GP11)

spi = busio.SPI(board.GP18, board.GP19, board.GP16)
esp = adafruit_esp32spi.ESP_SPIcontrol(spi, esp32_cs, esp32_ready, esp32_reset)


requests.set_socket(socket, esp)

if esp.status == adafruit_esp32spi.WL_IDLE_STATUS:
    print("ESP32 found and in idle mode")
print("Firmware vers.", esp.firmware_version)
print("MAC addr:", [hex(i) for i in esp.MAC_address])

print("Connecting to AP...")
while not esp.is_connected:
    try:
        led.value = True
        esp.connect_AP(secrets["ssid"], secrets["password"])
    except RuntimeError as e:
        led.value = False
        print("could not connect to AP, retrying... ")
        continue
print("Connected.")
print("My IP address is", esp.pretty_ip(esp.ip_address))
print(
    "IP lookup adafruit.com: %s" % esp.pretty_ip(esp.get_host_by_name("adafruit.com"))
)

username = 'diyelectromusic'

url = 'https://api.twitter.com/1.1/followers/list.json?cursor=-1&count=500&screen_name=' + username

# Set OAuth2.0 Bearer Token
bearer_token = secrets['twitter_bearer_token']
headers = {'Authorization': 'Bearer ' + bearer_token}

print ("Getting Twitter followers for", username)

led.value = True
response = requests.get(url, headers=headers)
led.value = False
json_data = response.json()
#with open ("twitfollow.json") as f:
#    json_data = json.load(f)

ids = []
for fid in json_data["users"]:
    ids.append(fid)

print (len(ids))

response.close()

ledvalue = True
while 1:
    for i in range (0, len(ids)):
        screen_name = ids[i]["screen_name"]
        followers = int(ids[i]["followers_count"])
        friends = int(ids[i]["friends_count"])
        print (screen_name,"\t(", followers, ", ", friends, ")")
        if (followers<500):
            delay = followers
        elif (followers<1000):
            delay = followers/2
        else:
            delay = followers/4
        midibytes = []
        while (friends > 0):
            midinote = int(friends) & 0x3f
            if (midinote != 0):
                midibytes.append(24 + midinote)
            friends = int (friends/64)

        led.value = ledvalue
        if (ledvalue):
            ledvalue = False
        else:
            ledvalue = True
        for mid in range (0, len(midibytes)):
            if midibytes[mid] != 0:
                noteOn(midibytes[mid])

        time.sleep(delay/1000)

        for mid in range (0, len(midibytes)):
            if midibytes[mid] != 0:
                noteOff(midibytes[mid])
