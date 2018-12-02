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

SFE_BMP180 PRESSURESENSOR;
#define ALTITUDE 250.0 //Stuttgart Center

#define DHTPIN 2
#define DHTVCC 7
#define DHTGND 6

#define BMPVCC 4
#define BMPGND 5
#define DUSTVCC 3

#define SLEEPTIME 1000 * 60 * 1 // 15 Minutes
#define DHTTYPE DHT22
#define COV_RATIO 0.2
#define NO_DUST_VOLTAGE 1.0
#define SYS_VOLTAGE 3300

#define UINT16_t_MAX 65536
#define INT16_t_MAX UINT16_t_MAX / 2

#define DEBUG true // true if debugging, this means no loop! it enables debug inputs and wait for serial

DHT dht(DHTPIN, DHTTYPE);
const int iled = 1;
const int vout = A1;

volatile int alarm_source = 0;
void alarmEvent0()
{
    alarm_source = 0;
}
void setup()
{
    pinMode(iled, OUTPUT);
    digitalWrite(iled, LOW);

    // making a Ground Pin for DHT
    pinMode(DHTGND, OUTPUT);
    digitalWrite(DHTGND, LOW);

    // making a Ground Pin for BMP
    pinMode(BMPGND, OUTPUT);
    digitalWrite(BMPGND, LOW);

    // making DHT,Dust and BMP VCC Pins
    pinMode(DHTVCC, OUTPUT);
    pinMode(BMPVCC, OUTPUT);
    pinMode(DUSTVCC, OUTPUT);
    //digitalWrite(BMPVCC, HIGH);
    //digitalWrite(DHTVCC, HIGH);
    //digitalWrite(DUSTVCC, HIGH);

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
    if (DEBUG == true)
    {
        Serial.println(message);
    }
    message.trim();
    /*
    for (int i = 0; i < 3; i++)
    {
        boolean send = sendStringAndGetResponse(message);
        if (send == true)
        {
            Serial.println("sending success");
            break;
        }
        Serial.println("sending again");
    }*/
    delay(10 * 1000); // we need time to get usb upload working, before it falls asleep
    if (DEBUG == false)
    {
        LowPower.sleep(SLEEPTIME);
    }
}

int16_t convertoFloatToInt16(float value, long max, long min)
{
    float conversionFactor = (float)(INT16_t_MAX) / (float)(max - min);
    return (int16_t)(value * conversionFactor);
}

uint16_t convertoFloatToUInt16(float value, long max, long min = 0)
{
    float conversionFactor = (float)(UINT16_t_MAX) / (float)(max - min);
    return (uint16_t)(value * conversionFactor);
}

String getWeather()
{
    String message = "";

    double pressure = getPressure();
    digitalWrite(DHTVCC, HIGH);
    delay(2000);
    dht.begin();
    float humidity = dht.readHumidity();
    digitalWrite(DHTVCC, LOW);
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
    Serial.println(humidity);
    Serial.println(pressure);
    Serial.println(density);
    message = message + Pad((int)(humidity * 10)); // Humidity
    message = message + Pad((int)(pressure * 10)); // Pressure
    message = message + Pad((int)(density * 10));  // Dust Density

    return message;
}

float getDust()
{
    digitalWrite(DUSTVCC, HIGH);
    delay(2000);
    float density, voltage;
    int adcvalue;
    digitalWrite(iled, LOW);
    delayMicroseconds(280);
    adcvalue = analogRead(vout);
    digitalWrite(iled, HIGH);
    adcvalue = Filter(adcvalue);
    voltage = (SYS_VOLTAGE / 1024.0) * adcvalue * 11.0;
    Serial.println(voltage);
    if (voltage >= NO_DUST_VOLTAGE)
    {
        voltage -= NO_DUST_VOLTAGE;
        density = voltage * COV_RATIO;
    }
    else
    {
        density = 0;
    }
    digitalWrite(DUSTVCC, LOW);
    return density;
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

int Filter(int m)
{
    static int flag_first = 0, _buff[10], sum;
    const int _buff_max = 10;
    int i;
    if (flag_first == 0)
    {
        flag_first = 1;
        for (i = 0, sum = 0; i < _buff_max; i++)
        {
            _buff[i] = m;
            sum += _buff[i];
        }
        return m;
    }
    else
    {
        sum -= _buff[0];
        for (i = 0; i < (_buff_max - 1); i++)
        {
            _buff[i] = _buff[i + 1];
        }
        _buff[9] = m;
        sum += _buff[9];
        i = sum / 10.0;
        return i;
    }
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
