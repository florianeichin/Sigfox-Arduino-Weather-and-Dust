/*
Needed Components:
----
Board: Arduino MKR Fox 1200

----
Temperature and Humidity: DHT 22 / Digital Pin 2
Pressure: BPM 810
Dust: Sharp gp2y1010au0f 

*/

#include "DHT.h"
#include <SigFox.h>
#include <ArduinoLowPower.h>
#include <SFE_BMP180.h>
#include <Wire.h>

SFE_BMP180 pressure;
#define ALTITUDE 350.0

#define DHTPIN 2    
#define DHTTYPE DHT22
#define        COV_RATIO                       0.2
#define        NO_DUST_VOLTAGE                 400
#define        SYS_VOLTAGE                     5000    


       
DHT dht(DHTPIN, DHTTYPE);
const int iled = 1;
const int vout = 1;
float density, voltage;
int   adcvalue;
 
void setup() 
{
  pinMode(iled, OUTPUT);
  digitalWrite(iled, LOW);
  Serial.begin(9600); 

  if (pressure.begin())
    Serial.println("BMP180 init success");
  else
  {
    Serial.println("BMP180 init fail\n\n");
    while(1); 
  }
  
  dht.begin();
  while (!Serial) {};
  if (!SigFox.begin()) {
    Serial.println("Shield error or not present!");
    return;
  }
  SigFox.debug();
  Serial.println("SigFox FW version " + SigFox.SigVersion());
  Serial.println("ID  = " + SigFox.ID());
  Serial.println("PAC = " + SigFox.PAC());
  delay(100);
  SigFox.end();
}
 
void loop() 
{
  Serial.println(getWeather(false));
  String message = getWeather(true);
  Serial.println("Sending " + message);
  message.trim();
  sendStringAndGetResponse(message);
  delay(900000); //15minute intervall
}

/*Helper Functions*/
String getWeather(boolean shrt){

  char status;
  double T,P,p0,a;
  status = pressure.startTemperature();
  if (status != 0)
  {
    delay(status);
    status = pressure.getTemperature(T);
    if (status != 0)
    {
      status = pressure.startPressure(3);
      if (status != 0)
      {
        delay(status);  
        status = pressure.getPressure(P,T);
        if (status != 0)
        {

          p0 = pressure.sealevel(P,ALTITUDE);
        }
      }
    }
  }


  
  String message;
  String nm;
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  digitalWrite(iled, HIGH);
  delayMicroseconds(280);
  adcvalue = analogRead(vout);
  digitalWrite(iled, LOW);
  adcvalue = Filter(adcvalue);
  voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11;
  if(voltage >= NO_DUST_VOLTAGE)
  {
    voltage -= NO_DUST_VOLTAGE;
    density = voltage * COV_RATIO;
  }
  else
    density = 0;

  if(shrt==true){
    message = "";
    nm=(int)(h*10);
    if(nm.length()<4){
      nm=0+nm;
    }
    message=message+ nm;
    String sign;
    if(t<0){
      sign="0";
    }
    else{
      sign="1";
    }
    nm=(int)(t*10);
    nm=sign+nm;
    if(nm.length()<4){
      nm=0+nm;
    }
    message=message+nm;
    nm=(int)(density*10);
    if(nm.length()<4){
      nm=0+nm;
    }
    message=message+nm;
    }
  else{
    message = "Humidity: ";
    message=message+h ;
    message=message+" %\t Temperature: "; 
    message=message+t;
    message=message+" C\t Dust Density: ";
    message=message+density;
    message=message+" ug/m3\t Sealevel Pressure: ";  
    message=message+p0;
    message = message+" hPa, \n";
  }

  return message;

}

int Filter(int m){
  static int flag_first = 0, _buff[10], sum;
  const int _buff_max = 10;
  int i;
  if(flag_first == 0)
  {
    flag_first = 1;
    for(i = 0, sum = 0; i < _buff_max; i++)
    {
      _buff[i] = m;
      sum += _buff[i];
    }
    return m;
  }
  else
  {
    sum -= _buff[0];
    for(i = 0; i < (_buff_max - 1); i++)
    {
      _buff[i] = _buff[i + 1];
    }
    _buff[9] = m;
    sum += _buff[9];
    i = sum / 10.0;
    return i;
  }
}

void sendStringAndGetResponse(String str) {
  SigFox.begin();
  delay(100);
  SigFox.status();
  delay(1);
  SigFox.beginPacket();
  SigFox.print(str);
  int ret = SigFox.endPacket(true);
  if (ret > 0) {
    Serial.println("No transmission");
  } else {
    Serial.println("Transmission ok");
  }

  Serial.println(SigFox.status(SIGFOX));
  Serial.println(SigFox.status(ATMEL));

  if (SigFox.parsePacket()) {
    Serial.println("Response from server:");
    while (SigFox.available()) {
      Serial.print("0x");
      Serial.println(SigFox.read(), HEX);
    }
  } else {
    Serial.println("Could not get any response from the server");
  }
  Serial.println();
  SigFox.end();
}