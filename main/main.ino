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


volatile int alarm_source = 0;
void alarmEvent0(){
  alarm_source = 0;
}
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
  String message = getWeather(true);
  if (ONESHOT == true){
    delay(2000);
    Serial.println(getWeather(false));
    Serial.println(message);
  }
  powerDown();
  message.trim();
  for(int i =0 ; i<3;i++){
    boolean send = sendStringAndGetResponse(message);
    if (send == true){
      Serial.println("sending success");
      break;
    }
    Serial.println("sending again");
  }
  
  if (ONESHOT == true) {
    delay(10*1000); // wait 10 seconds
  }else{
    LowPower.sleep(SLEEPTIME);  
  }
}


void powerUp(){
  dht.begin();
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
  double T,P,p0,a, hPa;
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
            hPa = p0/100;
          }
        }
      }
    }
    
  
  String message = "";
  float humidity = dht.readHumidity();

  // Read Dust Sensor
  float density; 
  float voltage;
  float adcvalue;
  digitalWrite(iled, HIGH);
  delayMicroseconds(280);
  adcvalue = analogRead(vout);
  digitalWrite(iled, LOW);
  adcvalue = Filter(adcvalue);
  voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11.0;
  if(voltage >= NO_DUST_VOLTAGE)
  {
    voltage -= NO_DUST_VOLTAGE;
    density = voltage * COV_RATIO;
  }
  else{
    density = 0;
  }
    
if (humidity != humidity){
  humidity = 0;
}
if (hPa != hPa){
  hPa = 0;
}
if (density != density){
  density = 0;
}
// create short message for sigfox delivery
  if(shrt==true){ 
    message = message + Pad((int)(humidity*10)); // Humidity
    message = message + Pad((int)(hPa*10)); // Pressure
    message = message + Pad((int)(density*10)); // Dust Density
  }

  else{ // create long message for serial output
    message = "Humidity: ";
    message = message + humidity ;
    message = message + " %\t Pressure: "; 
    message = message + hPa;
    message = message + " hPa\t Dust Density: ";
    message = message + density;
    message = message + " ug/m3\n";
  }
  return message;
}

// Byte Padding to 4 Byte
String Pad(int input){
      String message = (String)input;
     if(message.length()<4){
      message=0+message;
    } else if(message.length()>4){
      message = message.substring(0,4);
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

boolean sendStringAndGetResponse(String str) {
  SigFox.begin();
  delay(100);
  SigFox.status();
  delay(1);
  SigFox.beginPacket();
  SigFox.print(str);
  int ret = SigFox.endPacket(true);
  if (ret > 0) {
    Serial.println("No transmission");
    Serial.println();
    SigFox.end();
    return false;
  } else {
    Serial.println("Transmission ok");
  }
  if (ONESHOT==true){
    if (SigFox.parsePacket()) {
    Serial.println("Response from server:");
    while (SigFox.available()) {
      //Serial.print("0x");
      Serial.println(SigFox.read(),HEX);
    }
  }
  }
  Serial.println();
  SigFox.end();
  return true;
}
