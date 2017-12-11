/*####################################################################
 FILE: dht11.ino
 VERSION: 1.0
 PURPOSE: DHT11 Humidity and Temperature Digital Sensor test sketch

 Description:
 * Read temperature and humidity informations through DATA line and
 * print them on Serial Monitor.

 Connections:
 * BOARD -> ARDUINO
 * +5V   -> 5V
 * GND   -> GND
 * DATA  -> 2

 HISTORY:
 Mirko Prosseda - Original version (21/07/2013)
#######################################################################*/


#include <dht11.h>

dht11 DHT11;

#define DHT11PIN 2

void setup()
{
  Serial.begin(9600);
  Serial.println("DHT11 Humidity and Temperature Digital Sensor test sketch");
  delay(1000);          // wait for sensor initialization

}

void loop()
{
  uint8_t chk = DHT11.read(DHT11PIN);

  Serial.print("Sensor status: ");
  switch (chk)
  {
    case 0:  Serial.println("OK"); break;
    case -1: Serial.println("Checksum error"); break;
    case -2: Serial.println("Time out error"); break;
    case -3: Serial.println("The sensor is busy"); break;
    default: Serial.println("Unknown error"); break;
  }

  Serial.print("Humidity (%): ");
  Serial.println(DHT11.humidity, DEC);

  Serial.print("Temperature (C): ");
  Serial.println(DHT11.temperature, DEC);

  delay(2000);
}
