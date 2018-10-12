# Arduino Sigfox Weather 
Weather and Dust Values send to Sigfox Backend in an 15 Minute intervall.

Because of a bug, the LowPower.sleep() just works if the programmer is unset, so after programming the arduino, unplug the usb and plug it in again.

# Project Parts
## Board
Arduino MKR Fox 1200
https://store.arduino.cc/arduino-mkrfox1200

## Sensors
- Temperature and Humidity: DHT 22
- Pressure: BPM 180
- Dust: Sharp gp2y1010au0f

## Libraries
- DHT Sensor Library
- BMP85
- Arduino Low Power
- Sigfox MKR

## Data Encoding
3 x 4 ASCII Characters
1. Humidity in form ABCD which converts to AB,CD % relative humidity
2. Temperature in form ABCD which conerts to BC,D °C A=0 is a positive and A=1 a negative Value
3. Dust density in form ABCD which converts to AB,CD mg/m³ density of dust

