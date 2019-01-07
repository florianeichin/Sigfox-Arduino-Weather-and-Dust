/*
  Needed Components:
  ----
  Board: Arduino MKR Fox 1200

  ----
  Temperature and Humidity: DHT 22 / Digital Pin 2
  Pressure: BMP 810
  Dust: Sharp gp2y1010au0f

*/

#include <DHT.h>
#include <SigFox.h>
#include <ArduinoLowPower.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <SDS011-select-serial.h>

SFE_BMP180 PRESSURESENSOR;
#define ALTITUDE 250.0 //Stuttgart Center

#define DHTPIN 2
#define DHTVCC 7
#define DHTGND 6

#define BMPVCC 4
#define BMPGND 5

#define SLEEPTIME 1000 * 60 * 15 // 15 Minutes
#define DHTTYPE DHT22


#define DEBUG false // true if debugging, this means no loop! it enables debug inputs and wait for serial

DHT dht(DHTPIN, DHTTYPE);
SDS011 my_sds(Serial1);

volatile int alarm_source = 0;
void alarmEvent0()
{
  alarm_source = 0;
}
void setup()
{
  // making a Ground Pin for DHT
  pinMode(DHTGND, OUTPUT);
  digitalWrite(DHTGND, LOW);

  // making a Ground Pin for BMP
  pinMode(BMPGND, OUTPUT);
  digitalWrite(BMPGND, LOW);

  // making DHT and BMP VCC Pins
  pinMode(DHTVCC, OUTPUT);
  pinMode(BMPVCC, OUTPUT);


  Serial.begin(9600);
  delay(2000);

  if (!SigFox.begin())
  {
    Serial.println("Shield error or not present!");
    return;
  }
  if (DEBUG == true)
  {
    while (!Serial)
    {
    }; //if in debugging phase, this prevents a start before serial monitor was opened!
    SigFox.debug();
  }
  else
  {
    SigFox.noDebug();
  }
  delay(100);
  SigFox.end();
  LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, alarmEvent0, CHANGE);
}

void loop()
{
  String message = getWeather();
  Serial.println("Message: " + String(message));
  message.trim();

  for (int i = 0; i < 3; i++)
  {
    boolean send = sendStringAndGetResponse(message);
    if (send == true)
    {
      Serial.println("sending success");
      break;
    }
    Serial.println("sending again");
  }
  delay(10 * 1000); // we need time to get usb upload working, before it falls asleep
  if (DEBUG == false)
  {
    LowPower.sleep(SLEEPTIME);
  }
}



String getWeather()
{
  String message = "";

  // read pressure
  double pressure = getPressure();

  // read Humidity
  digitalWrite(DHTVCC, HIGH);
  delay(2000);
  dht.begin();
  float humidity = dht.readHumidity();
  digitalWrite(DHTVCC, LOW);

  // read dust
  float density = getDust();

  if (humidity != humidity)
  { // nan value chacking
    humidity = 0;
  }
  if (pressure != pressure)
  { // nan value chacking
    pressure = 0;
  }
  if (density != density)
  { // if nan value
    density = 0;
  }
  // create short message for sigfox delivery
  Serial.println("PHumidity: " + String(humidity));
  Serial.println("Pressure: " + String(pressure));
  Serial.println("Density: " + String(density));
  message = message + Pad((int)(humidity * 10)); // Humidity
  message = message + Pad((int)(pressure * 10)); // Pressure
  message = message + Pad((int)(density * 10));  // Dust Density

  return message;
}

float getDust()
{
  float p10, p25;
  int error;
  Serial1.begin(9600);
  delay(1000);
  error = my_sds.read(&p25, &p10);
  if (! error) {
    Serial.println("P2.5: " + String(p25));
    Serial.println("P10:  " + String(p10));
  }
  delay(1000);
  return p10;
}

double getPressure()
{
  digitalWrite(BMPVCC, HIGH);
  delay(2000);
  PRESSURESENSOR.begin();

  char status;
  double T, P, p0, a, hPa;
  status = PRESSURESENSOR.startTemperature();
  if (status != 0)
  {
    delay(1000);
    status = PRESSURESENSOR.getTemperature(T);
    if (status != 0)
    {
      status = PRESSURESENSOR.startPressure(3);
      if (status != 0)
      {
        delay(1000);
        status = PRESSURESENSOR.getPressure(P, T);
        if (status != 0)
        {
          p0 = PRESSURESENSOR.sealevel(P, ALTITUDE);
          hPa = p0 / 100;
        }
      }
    }
  }
  digitalWrite(BMPVCC, LOW);

  return hPa;
}

// Byte Padding to 4 Byte
String Pad(int input)
{
  String message = (String)input;
  while (message.length() < 4)
  {
    message = 0 + message;
  }
  if (message.length() > 4)
  {
    message = "2222";
  }
  return message;
}


boolean sendStringAndGetResponse(String str)
{
  SigFox.begin();
  delay(100);
  SigFox.status();
  delay(1);
  SigFox.beginPacket();
  SigFox.print(str);
  int ret = SigFox.endPacket(true);
  if (ret > 0)
  {
    Serial.println("No transmission");
    Serial.println();
    SigFox.end();
    return false;
  }
  else
  {
    Serial.println("Transmission ok");
  }
  if (DEBUG == true)
  {
    if (SigFox.parsePacket())
    {
      Serial.println("Response from server:");
      while (SigFox.available())
      {
        Serial.println(SigFox.read(), HEX);
      }
    }
  }
  Serial.println();
  SigFox.end();
  return true;
}
