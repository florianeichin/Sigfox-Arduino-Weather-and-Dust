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

SFE_BMP180 pressure;
#define 	ALTITUDE 250.0 //Stuttgart Center

#define 	     DHTPIN                         2    
#define 	     DHTVCC                         7    
#define 	     DHTGND                         6    

#define 	     BMPVCC                         4    
#define 	     BMPGND                         5  
#define        DUSTVCC                        3 

#define       SLEEPTIME                       1000*60*15 // 15 Minutes
#define 	     DHTTYPE                        DHT22
#define        COV_RATIO                       0.2
#define        NO_DUST_VOLTAGE                 1.0
#define        SYS_VOLTAGE                     3300.0    

#define         UINT16_t_MAX                   65536
#define         INT16_t_MAX                    UINT16_t_MAX/2

#define       ONESHOT                       false // true if debugging, this means no loop! it enables debug inputs and wait for serial
       
DHT dht(DHTPIN, DHTTYPE);
const int iled = 1;
const int vout = 1;
float density, voltage;
float   adcvalue;
volatile int alarm_source = 0;
 
void setup() 
{
  pinMode(iled, OUTPUT);
  digitalWrite(iled, LOW);

  // making a Ground Pin for DHT
  pinMode(DHTGND,OUTPUT);
  digitalWrite(DHTGND,LOW);
  
  // making a Ground Pin for BMP
  pinMode(BMPGND,OUTPUT);
  digitalWrite(BMPGND,LOW);

  // making DHT,Dust and BMP VCC Pins
  pinMode(DHTVCC,OUTPUT);
  pinMode(BMPVCC,OUTPUT);
  pinMode(DUSTVCC,OUTPUT);
  digitalWrite(BMPVCC,HIGH);
  digitalWrite(DHTVCC,HIGH);
  digitalWrite(DUSTVCC,HIGH);  
  
  Serial.begin(9600); 
  delay(2000);

  if (pressure.begin()){
    Serial.println("BMP180 init success");
  }
  else
  {
    Serial.println("BMP180 init fail\n\n");
    while(1); 
  }
  
  dht.begin();
  
  if (!SigFox.begin()) {
    Serial.println("Shield error or not present!");
    return;
  }
  if(ONESHOT==true){
    while (!Serial) {}; //if debugging, this prevents a start before serial monitor was opened!
    SigFox.debug();
  }else{
    SigFox.noDebug();
  }
  delay(100);
  SigFox.end();
  LowPower.attachInterruptWakeup(RTC_ALARM_WAKEUP, alarmEvent0, CHANGE); // Needed to fix LowPower Library Bug
}
 
void loop() 
{
  powerUp();
  Serial.println(getWeather(false));
  String message = getWeather(true);
  powerDown();
  message.trim();
  sendStringAndGetResponse(message);
  if (ONESHOT == true) {
    // spin forever, so we can test that the backend is behaving correctly
    while (1) {}
  }
  LowPower.sleep(SLEEPTIME);
}


void powerUp(){
  digitalWrite(BMPVCC,HIGH);
  digitalWrite(DHTVCC,HIGH);
  digitalWrite(DUSTVCC,HIGH);
  
}

void powerDown(){
  digitalWrite(BMPVCC,LOW);
  digitalWrite(DHTVCC,LOW);
  digitalWrite(DUSTVCC,LOW);
}

int16_t convertoFloatToInt16(float value, long max, long min) {
  float conversionFactor = (float) (INT16_t_MAX) / (float)(max - min);
  return (int16_t)(value * conversionFactor);
}

uint16_t convertoFloatToUInt16(float value, long max, long min = 0) {
  float conversionFactor = (float) (UINT16_t_MAX) / (float)(max - min);
  return (uint16_t)(value * conversionFactor);
}

String getWeather(boolean shrt){
  char status;
  double T,P,p0,a;
  if(shrt==false){ // Read Pressure only if needed
    status = pressure.startTemperature();
    if (status != 0){
      delay(status);
      status = pressure.getTemperature(T);
      if (status != 0){
        status = pressure.startPressure(3);
        if (status != 0){
          delay(status);  
          status = pressure.getPressure(P,T);
          if (status != 0){
            p0 = pressure.sealevel(P,ALTITUDE);
          }
        }
      }
    }
  }
  

  
  String message = "";
  String newMessage;
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Read Dust Sensor
  digitalWrite(iled, HIGH);
  delayMicroseconds(280);
  adcvalue = analogRead(vout);
  digitalWrite(iled, LOW);
  adcvalue = Filter(adcvalue);
  voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11.0;
  Serial.println(voltage);
  if(voltage >= NO_DUST_VOLTAGE)
  {
    voltage -= NO_DUST_VOLTAGE;
    density = voltage * COV_RATIO;
  }
  else{
    density = 0;
  }
    

// create short message for sigfox delivery
  if(shrt==true){ 
    message = "";

    // Humidity
    newMessage=(int)(humidity*10);
    if(newMessage.length()<4){ //4 Byte Padding
      newMessage=0+newMessage;
    }
    message=message+ newMessage;

    // Temperature
    String sign;
    if(temperature<0){
      sign="1";
    }
    else{
      sign="0";
    }
    newMessage=(int)(temperature*10);
    if(newMessage.length()<3){
      newMessage=0+newMessage;
    }
    message=message+sign+newMessage;
    
    // Dust Density
    newMessage=(int)(density*10);
    if(newMessage.length()<4){
      newMessage=0+newMessage;
    }
    message=message+newMessage;
    }

  else{ // create long message for serial output
    message = "Humidity: ";
    message=message+humidity ;
    message=message+" %\t Temperature: "; 
    message=message+temperature;
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
  Serial.println();
  SigFox.end();
  return;
}







// needed for BUG FIX of LowPower Sleep
void alarmEvent0() {
  alarm_source = 0;
}
