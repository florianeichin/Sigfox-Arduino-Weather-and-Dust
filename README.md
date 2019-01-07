# Arduino Sigfox Weather 
Weather and Dust Values send to Sigfox Backend in an 15 Minute intervall.

Because of a bug, the LowPower.sleep() just works if the programmer is unset, so after programming the arduino, unplug the usb and plug it in again.

yet the 5V just works with USB or regulated external power via vin, so no batteries

# Project Parts
## Board
Arduino MKR Fox 1200
https://store.arduino.cc/arduino-mkrfox1200

## Sensors
- Humidity and Humidity: DHT 22
- Pressure: BPM 180
- Dust: SDS011

## Libraries
- DHT Sensor Library
- BMP85
- Arduino Low Power
- Sigfox MKR
- SDS011 Select Serial

## Data Encoding
3 x 4 ASCII Characters
1. Humidity in form ABCD which converts to ABC,D % relative humidity
2. Pressure in form ABCD which conerts to ABC,D hPA
3. Dust density in form ABCD which converts to ABC,D mg/m³ density of P10 dust

