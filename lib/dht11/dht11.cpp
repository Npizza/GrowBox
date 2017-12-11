/*####################################################################
 FILE: dht11.cpp
 VERSION: 1.0
 PURPOSE: DHT11 Humidity and Temperature Digital Sensor library for Arduino

 HISTORY:
 Mirko Prosseda - Original version (21/07/2013)
#######################################################################*/


#include "dht11.h"

// Check the duration of a specific level status on the data line
int8_t dht11::chkdelay(uint8_t us, uint8_t status)
{
    pinMode(sensorPin, INPUT);
    unsigned long t = micros();
    while(digitalRead(sensorPin) == status) // wait for a status variation
        if ((micros() - t) > us) return 1;  // or a timeout
    return 0;
}


// returned values:
//  0 : OK
// -1 : checksum error
// -2 : timeout
// -3 : data line busy

// Read sensor data
int8_t dht11::read(uint8_t pin)
{
    uint8_t bytes[5]; // input buffer
    uint8_t cnt = 7;
    uint8_t idx = 0;
    uint8_t chksum;
    unsigned long t;

    sensorPin = pin;

    // Initialize the buffer
    for (uint8_t i=0; i< 5; i++) bytes[i] = 0;

    // Verify if the data line is busy
    if (!chkdelay(100, HIGH)) return -3; // data line busy

    // Send Start signal
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    delay(18);
    digitalWrite(pin, HIGH);
    delayMicroseconds(40);

    // Wait the Acknowledge signal
    if (chkdelay(100, LOW)) return -2;  // Timeout
    if (chkdelay(100, HIGH)) return -2; // Timeout

    // Read data output from the sensor
    for (int i=0; i<40; i++)
    {
        if (chkdelay(60, LOW)) return -2;  // Timeout
        t = micros();
        if (chkdelay(80, HIGH)) return -2; // Timeout

 	if ((micros() - t) > 40) bytes[idx] += (1 << cnt); // bit 1 received

 	if (cnt == 0)   // end of the byte
 	{
 	    cnt = 7;    // reset at MSB
 	    idx++;      // point to next byte
 	}
 	else cnt--;
    }

    // Update variables
    // bytes[1] and bytes[3] are allways zero
    humidity    = bytes[0];
    temperature = bytes[2];

    // Verify the Checksum value
    chksum = bytes[0] + bytes[2];
    if (chksum != bytes[4]) return -1; // checksum error

    return 0; // OK
}
