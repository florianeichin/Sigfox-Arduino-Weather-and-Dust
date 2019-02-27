#include <DHT.h>
#include <SigFox.h>
#include <ArduinoLowPower.h>
#include <SFE_BMP180.h>
#include <Wire.h>

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
  // read pressure
  double pressure = getPressure();

  // read Humidity

  dht.begin();
  delay(2000);
  //float humidity = dht.readHumidity();
  float humidity = 0;
  delay(2000);
  float temperature = dht.readTemperature();
  // read dust
  float density = 0;

  if (humidity != humidity)
  { // nan value chacking
    humidity = 0;
  }
  
  humidity = humidity * 10;
  char humidityHex[3];
  sprintf(humidityHex, "%03x", int(humidity));
    if (temperature != temperature)
  { // nan value chacking
    temperature = 0;
  }
  temperature = (temperature + 50) * 10;
  char temperatureHex[3];
  sprintf(temperatureHex, "%03x", int(temperature));

  if (pressure != pressure)
  { // nan value chacking
    pressure = 0;
  }
  char pressureHex[3];
  sprintf(pressureHex, "%03x", int(pressure));
  if (density != density)
  { // if nan value
    density = 0;
  }
  density = density * 10;
  char densityHex[3];
  sprintf(densityHex, "%03x", int(density));
  // create short message for sigfox delivery
  Serial.println("Humidity: " + String(humidity)+ " hex:" + String(humidityHex));
  Serial.println("Pressure: " + String(pressure)+ " hex:" + String(pressureHex));
  Serial.println("Density: " + String(density)+ " hex:" + String(densityHex));
  Serial.println("Temprerature: " + String(temperature)+ " hex:" + String(temperatureHex));
  char message[12];
  sprintf(message,"%s%s%s%s",humidityHex , pressureHex ,densityHex ,temperatureHex);
 
  return message;
}

double getPressure()
{
  digitalWrite(BMPVCC, HIGH);
  delay(2000);
  PRESSURESENSOR.begin();
  delay(2000);
  char status;
  double T, P, hPa;
  status = PRESSURESENSOR.startTemperature();
  delay(status);
  if (status != 0)
  {
    status = PRESSURESENSOR.getTemperature(T);
    if (status != 0)
    {
      status = PRESSURESENSOR.startPressure(3);
      delay(status);
      if (status != 0)
      {
        status = PRESSURESENSOR.getPressure(P, T);
        if (status != 0)
        {
          hPa = PRESSURESENSOR.sealevel(P, ALTITUDE);
        }
      }
    }
  }
  digitalWrite(BMPVCC, LOW);

  return hPa;
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
