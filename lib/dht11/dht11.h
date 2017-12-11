/*####################################################################
 FILE: dht11.h
 VERSION: 1.0
 PURPOSE: DHT11 Humidity and Temperature Digital Sensor library for Arduino

 HISTORY:
 Mirko Prosseda - Original version (21/07/2013)
#######################################################################*/


#ifndef dht11_h
#define dht11_h

#include "Arduino.h"


class dht11
{
private:
    uint8_t sensorPin;                           //default sensor pin
    int8_t chkdelay(uint8_t us, uint8_t status); // check level duration
public:
    int8_t read(uint8_t pin);                    // read sensor data
    uint8_t humidity;
    uint8_t temperature;
};
#endif
