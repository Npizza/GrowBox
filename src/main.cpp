#include <Arduino.h>
#include <DS1302RTC.h>  //RTC stuff
#include <Streaming.h>        //http://arduiniana.org/libraries/streaming/
#include <Time.h>             //http://playground.arduino.cc/Code/Time
#include <MenuSystem.h> //for the MenuSystem
#include <SPI.h>
#include <Adafruit_GFX.h> //libs for the lcd
#include <Adafruit_PCD8544.h>
#include <dht11.h> // for the humidity and temperature
#include <TimeAlarms.h>

/*ALARM SET UP FOR GROW LIGHT */
time_t timeNow;

#define PIN_LIGHT_1 22 // Relay IN5: Grow
#define PIN_PUMP_IRRIGATION 24 // Relay IN7: Water pump (for hydro-system)

/* 4-relay-board needs HIGH to OPEN the relay, so for readability, we redefine the use of HIGH/LOW */
#define RELAYON LOW
#define RELAYOFF HIGH

/* Light schemes in minutes */
// Light1: Growth light (12+1 method)
// => 12 hours ON, 5.5 hours OFF, 1 hour ON, 5.5 hours OFF, repeat
// The array HAS to sum up to 24 hours, but can be of any even count / growth light periods.
// Only two growth light periods (typically ON for 1080 minutes, OFF for 360 minutes) are
// also possible.
//boolean GrowLightStatus = true;
#define GROWTH_LIGHT_PERIODS 2
int Total_Minute_light_on_duration=1; //set minutes for light on
int Total_Minute_light_off_duration=1;  // set minutes for light off
int growthLightScheme[GROWTH_LIGHT_PERIODS] = { Total_Minute_light_on_duration, Total_Minute_light_off_duration };
byte currentGrowthLightPeriod;

// Time of day growth light scheme should start (example: 4:20 pm)
byte growthLightStartTimeHour = 16;
byte growthLightStartTimeMinute = 20;

long secondsToNextGrowthLightSwitch = 0; // Calculated on startup to set first planned light switch

void turnOnGrowthLight(); //it has declaretion problems (atoms probably fault)

void getGrowthLightPeriod() {
  // Create time element for Alarm setup of growth light
  time_t timeNow = now();
  tmElements_t dailyGrowthLightStartTime;
  breakTime(timeNow, dailyGrowthLightStartTime);
  // Modify time element with variable start time (default: 16:20)
  dailyGrowthLightStartTime.Hour = growthLightStartTimeHour;
  dailyGrowthLightStartTime.Minute = growthLightStartTimeMinute;
  dailyGrowthLightStartTime.Second = 0;

  // Subtract one day from growth light start time
  // Set up variable to count through growth light periods
  unsigned long sumOfGrowthLightPeriods = makeTime(dailyGrowthLightStartTime) - SECS_PER_DAY;
  byte count = 0;

  // Comb through on/off periods since one day earlier until we know which lighting period we're at now
  while (sumOfGrowthLightPeriods <= timeNow) {
    sumOfGrowthLightPeriods += growthLightScheme[count] * SECS_PER_MIN;
    if (sumOfGrowthLightPeriods <= timeNow) {  // If already past now(), don't count next light period
      currentGrowthLightPeriod = count;
      if (count < GROWTH_LIGHT_PERIODS-1) count++;
      else count = 0;
    }
  }
  secondsToNextGrowthLightSwitch = sumOfGrowthLightPeriods - timeNow;
  currentGrowthLightPeriod = count;
}

void turnOffGrowthLight() {
  digitalWrite(PIN_LIGHT_1, RELAYOFF);
  Serial.print(F("Turned off growth light"));
  //GrowLightStatus=false;
  Serial.print(F(" at "));
  Serial.println(now());

  getGrowthLightPeriod();
  Alarm.timerOnce(secondsToNextGrowthLightSwitch, turnOnGrowthLight);
  Serial.print(F("Set up growth light ON in seconds: "));
  Serial.println(secondsToNextGrowthLightSwitch);
}

void turnOnGrowthLight() {
  digitalWrite(PIN_LIGHT_1, RELAYON);
  //GrowLightStatus=true;
  Serial.print(F("Turned on growth light"));
  Serial.print(" at ");
  Serial.println(now());
  getGrowthLightPeriod();
  Alarm.timerOnce(secondsToNextGrowthLightSwitch, turnOffGrowthLight);
  Serial.print(F("Set up growth light OFF in seconds: "));
  Serial.println(secondsToNextGrowthLightSwitch);
}

/*Alarm water*/
/* Irrigation scheme in seconds */
// => 0.5 minutes on, 10 minutes off, repeat
unsigned long Total_Minute_water_on_duration = 1;  // set minute for water on
unsigned long Total_Minute_water_off_duration = 1;  //  set minute for water off
unsigned long irrigationScheme[2] = { Total_Minute_water_on_duration, Total_Minute_water_off_duration };
void turnOffIrrigation(); // problem with declaretion
void turnOnIrrigation() {
  digitalWrite(PIN_PUMP_IRRIGATION, RELAYON);
  Alarm.timerOnce(irrigationScheme[0], turnOffIrrigation);
  Serial.print(F("Turned on irrigation at "));
  Serial.println(now());
}

void turnOffIrrigation() {
  digitalWrite(PIN_PUMP_IRRIGATION, RELAYOFF);
  Alarm.timerOnce(irrigationScheme[1], turnOnIrrigation);
  Serial.print(F("Turned off irrigation at "));
  Serial.println(now());
}

/*Sensors staff*/
/*####################################################################
 Connections:
 * BOARD -> ARDUINO
 * +5V   -> 5V
 * GND   -> GND
 * DATA  -> 2
#######################################################################*/
dht11 DHT11;
#define DHT11PIN 2  //pin of the sensor
int Temperature,Humidity; //global var for keep temp and humidity
const int hygrometerPin = A2;	//Hygrometer sensor analog pin output at pin A0 of Arduino
int SoilMu; //Value stores the Soil humidity

void GetTempHum(){
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


  Temperature=DHT11.temperature;
  Humidity=DHT11.humidity;

  Serial.print("Humidity (%): ");
  Serial.println(Humidity, DEC);

  Serial.print("Temperature (C): ");
  Serial.println(Temperature, DEC);

}

void GetSoilMu(){
  SoilMu = analogRead(hygrometerPin);		//Read analog SoilMu
  SoilMu = constrain(SoilMu,400,1023);	//Keep the ranges!
  SoilMu = map(SoilMu,400,1023,100,0);	//Map SoilMu : 400 will be 100 and 1023 will be 0
  Serial.print("Soil humidity: ");
  Serial.print(SoilMu);
  Serial.println("%");

}

#define PCD8544_CHAR_HEIGHT 8
//for the pin check the link
//http://cyaninfinite.com/tutorials/nokia-screen-for-display/
// LCD
// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 lcd = Adafruit_PCD8544(13, 11, 5, 7, 6);

/*******************Global variables for set the light,water etc...*****************************/
//duration time for lights on
int Day_light_on_duration=0;
int Hour_light_on_duration=0;
int Minute_light_on_duration=0;
//duration time for lights off
int Day_light_off_duration=0;
int Hour_light_off_duration=0;
int Minute_light_off_duration=0;
//duration time for water on
int Day_water_on_duration=0;
int Hour_water_on_duration=0;
int Minute_water_on_duration=0;
//duration time for water off
int Day_water_off_duration=0;
int Hour_water_off_duration=0;
int Minute_water_off_duration=0;
/********************************************************/

/********RTC STAFF**********/
// Set pins:  CE, IO,CLK
DS1302RTC RTC(53, 51, 49);
// Optional connection for RTC module
#define DS1302_GND_PIN 47
#define DS1302_VCC_PIN 45

tmElements_t tm; /*for the timefuction*/
time_t t;

//set the RTC on te setup()
void setupRTC(){
  //======Start of RTC stuff =====
  // Activate RTC module
  pinMode(DS1302_GND_PIN, OUTPUT);
  pinMode(DS1302_VCC_PIN, OUTPUT);

  digitalWrite(DS1302_GND_PIN, LOW);
  digitalWrite(DS1302_VCC_PIN, HIGH);
  Serial << F("RTC module activated");
  Serial << endl;
  Alarm.delay(500);
  if (RTC.haltRTC()) {
    Serial << F("The DS1302 is stopped.  Please set time");
    Serial << F("to initialize the time and begin running.");
    Serial << endl;
  }
  if (!RTC.writeEN()) {
    Serial << F("The DS1302 is write protected. This normal.");
    Serial << endl;
  }
  Alarm.delay(1000);
  //setSyncProvider() causes the Time library to synchronize with the
  //external RTC by calling RTC.get() every five mins by default.
  setSyncProvider(RTC.get);
  Serial << F("RTC Sync");
  if (timeStatus() == timeSet)
    Serial << F(" Ok!");
  else
    Serial << F(" FAIL!");
  Serial << endl;

  //======End of RTC stuff =====

}   //set up the rtc stuff
void printI00(int val, char delim)
{
    if (val < 10) Serial << '0';
    Serial << _DEC(val);
    if (delim > 0) Serial << delim;
    return;
}
void print2digits(int number) {
  if (number >= 0 && number < 10)
    Serial.write('0');
  Serial.print(number);
}
//print date to Serial
void printDate(time_t t)
{
    printI00(day(t), 0);
    Serial << monthShortStr(month(t)) << _DEC(year(t));
}
//print time to Serial
void printTime(time_t t)
{
    printI00(hour(t), ':');
    printI00(minute(t), ':');
    printI00(second(t), ' ');
}
//print date and time to Serial
void printDateTime(time_t t)
{
    printDate(t);
    Serial << ' ';
    printTime(t);
}

/*RTC fuctions for LCD*/
void LCDprintI00(int val, char delim)
{
    if (val < 10) lcd.print("0");
    lcd.print(val);
    if (delim > 0) lcd.print(delim);
    return;
}
void LCDprintDate(time_t t)
{
    LCDprintI00(day(t), 0);
    lcd.print( monthShortStr(month(t)));
    lcd.println(year(t));
}
//print time to Serial
void LCDprintTime(time_t t)
{
    LCDprintI00(hour(t), ':');
    LCDprintI00(minute(t), ':');
    LCDprintI00(second(t), ' ');
}
//print date and time to Serial
void LCDprintDateTime(time_t t)
{
    LCDprintDate(t);
    lcd.print(" ");
    LCDprintTime(t);
}
/**********END OF RTC STUFF*****/

//Globalize variables
int sec, min, ora, weekDay, monDay, mon, y;

/***********JOYSTICK STUFF*****************************/
int JoyStick_X = A0; // x
int JoyStick_Y = A1; // y
int JoyStick_Z = 52; // key
int readJoystick() {    //JOYSTICK HANDLER
  int x, y, z;
  int m;
  Serial.print("readJoystick\n" );
  x = analogRead (JoyStick_X);
  y = analogRead (JoyStick_Y);
  z = digitalRead (JoyStick_Z);
  Alarm.delay(100);
  if (x < 100 && y > 500 && y < 550) {  // when m=1 joystick is UP
    Serial.print (x, DEC);
    Serial.print (",");
    Serial.print (y, DEC);
    Serial.print (",");
    Serial.println (z, DEC);
    Serial.println("UP");
    m=1;
    return m;

  }
  else if(x>950 && y>500 && y<550){   // when m=2 joystick is DOWN
    Serial.print (x, DEC);
    Serial.print (",");
    Serial.print (y, DEC);
    Serial.print (",");
    Serial.println (z, DEC);
    Serial.println("DOWN");
    m=2;
    return m;
  }
  else if(x>500 && x<550 && y>950){   //when m=3 joystick is LEFT
    Serial.print (x, DEC);
    Serial.print (",");
    Serial.print (y, DEC);
    Serial.print (",");
    Serial.println (z, DEC);
    Serial.println("LEFT");
    m=3;
    return m;
  }
  else if(x>500 && x<550 && y<100){   //when m=4 joystick is RIGHT
    Serial.print (x, DEC);
    Serial.print (",");
    Serial.print (y, DEC);
    Serial.print (",");
    Serial.println (z, DEC);
    Serial.println("RIGHT");
    m=4;
    return m;

  }else if( z == LOW){
    Serial.println("Pressed") ;       //when m=5 joystick is pressed
    m=5;
    return m;
  }
  else if(z== HIGH){
    m=0;
    return m;
  }
}
/***********END OF JOYSTICK STUFF*******************************/
// Renderer
class MyRenderer : public MenuComponentRenderer
{
  public:
    virtual void render(Menu const& menu) const
    {
      lcd.clearDisplay();
      menu.render(*this);
      menu.get_current_component()->render(*this);
      lcd.display();
    }

    virtual void render_menu_item(MenuItem const& menu_item) const
    {
      lcd.setCursor(0, 1 * PCD8544_CHAR_HEIGHT);
      lcd.print(menu_item.get_name());
    }

    virtual void render_back_menu_item(BackMenuItem const& menu_item) const
    {
      lcd.setCursor(0, 1 * PCD8544_CHAR_HEIGHT);
      lcd.print(menu_item.get_name());
    }

    virtual void render_numeric_menu_item(NumericMenuItem const& menu_item) const
    {
      lcd.setCursor(0, 1 * PCD8544_CHAR_HEIGHT);
      lcd.print(menu_item.get_name());
    }

    virtual void render_menu(Menu const& menu) const
    {
      lcd.setCursor(0, 0 * PCD8544_CHAR_HEIGHT);
      lcd.print(menu.get_name());
    }
};
MyRenderer my_renderer; //this is for the main menu
// Menu callback function

void info_selected(MenuItem* p_menu_item)
{

  while(digitalRead(JoyStick_Z) == HIGH) {
    GetTempHum(); //get the values from the sensor
    GetSoilMu();
    //get the time
    static time_t tLast;
    time_t t;
    t = now();
    if (t != tLast) {
        tLast = t;
        printDateTime(t);
        Serial << endl;
    }
    lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
    lcd.clearDisplay();              // clears the screen and buffer
    lcd.setCursor(0, 0);
    lcd.setTextSize(1);     // set text size
    LCDprintDateTime(t);
    lcd.println("");
    lcd.print(Temperature);
    lcd.print("C ");
    lcd.print(Humidity);
    lcd.println("% HUM");
    lcd.print("Light: ");
    lcd.println(4);
    lcd.print("Cooling: ");
    lcd.println(6);
    lcd.print("Soil: ");
    lcd.print(SoilMu);
    lcd.print("%");
    lcd.display();
    Alarm.delay(700);
   };
  Alarm.delay(10); // so we can look the result on the LCD
}

void Set_light_on(MenuItem* p_menu_item)
{
    int counter=0;
    int temp=0;
    while (temp <= 4) {
    lcd.setCursor(0, 1 * PCD8544_CHAR_HEIGHT);
    lcd.println("Light ON for: ");
    temp = readJoystick();
    if (counter == 0) {
      if (temp == 4) {    //if dexia is pressed(dexia)

        Day_light_on_duration= Day_light_on_duration + 1;
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(">");
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }
      else if(temp == 3) {     // if right is pressed(aristera)
        Day_light_on_duration= Day_light_on_duration - 1;
        if (Day_light_on_duration <= 0 ) {
            Day_light_on_duration=0;
        }
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(">");
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }else if (temp==2) {   //if the down is pressed
        counter++;
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(">");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
      }else if (temp==1) {   //if the up is pressed
        counter--;
        if (counter <= 0) {
          counter = 0;
        }
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(">");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
      }
      else{
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(">");
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }
    }else if (counter == 1) {
      if (temp == 4) {    //if left is pressed(dexia)
        Hour_light_on_duration= Hour_light_on_duration + 1;
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(">");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }
      else if(temp == 3) {     // if right is pressed(aristera)
        Hour_light_on_duration= Hour_light_on_duration - 1;
        if (Hour_light_on_duration <= 0 ) {
            Hour_light_on_duration=0;
        }
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(">");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }else if (temp ==2) {   //if the down is pressed
        counter++;
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(">");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }else if (temp ==1) {   //if the up is pressed
        counter--;
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(">");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }
      else{
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(">");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }
    }else if (counter == 2) {
      if (temp == 4) {    //if left is pressed(dexia)
        Minute_light_on_duration= Minute_light_on_duration + 1;
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(">");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }
      else if(temp == 3) {     // if right is pressed(aristera)
        Minute_light_on_duration= Minute_light_on_duration - 1;
        if (Minute_light_on_duration <= 0 ) {
            Minute_light_on_duration=0;
        }
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(">");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }else if (temp==1) {   //if the up is pressed
        counter--;
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }else if (temp==2) {   //if the down is pressed
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }
      else{
        lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
        lcd.print(Day_light_on_duration);
        lcd.println(" Days ");
        lcd.print(Hour_light_on_duration);
        lcd.println(" Hours");
        lcd.print(">");
        lcd.print(Minute_light_on_duration);
        lcd.println(" Minute");
        lcd.display();
      }

    }
  lcd.clearDisplay();
  Alarm.delay(100);
  }
  //find the total minutes and set the values4=
  Total_Minute_light_on_duration = Day_light_on_duration * 24 * 60 + Hour_light_on_duration * 60 + Minute_light_on_duration;
  growthLightScheme[0] =Total_Minute_light_on_duration;
}


void Set_light_off(MenuItem* p_menu_item)
{

    int counter=0;    //counts when the button is pressed
    int temp=0;
    while (temp <= 4) {
      lcd.setCursor(0, 1 * PCD8544_CHAR_HEIGHT);
      lcd.println("Light OFF for: ");
      temp = readJoystick();
      if (counter == 0) {
        if (temp == 4) {    //if dexia is pressed(dexia)

          Day_light_off_duration= Day_light_off_duration + 1;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(">");
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }
        else if(temp == 3) {     // if right is pressed(aristera)
          Day_light_off_duration= Day_light_off_duration - 1;
          if (Day_light_off_duration <= 0 ) {
              Day_light_off_duration=0;
          }
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(">");
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }else if (temp==2) {   //if the down is pressed
          counter++;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
        }else if (temp==1) {   //if the up is pressed
          counter--;
          if (counter <= 0) {
            counter = 0;
          }
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
        }
        else{
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(">");
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }
      }else if (counter == 1) {
        if (temp == 4) {    //if left is pressed(dexia)
          Hour_light_off_duration= Hour_light_off_duration + 1;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }
        else if(temp == 3) {     // if right is pressed(aristera)
          Hour_light_off_duration= Hour_light_off_duration - 1;
          if (Hour_light_off_duration <= 0 ) {
              Hour_light_off_duration=0;
          }
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }else if (temp ==2) {   //if the down is pressed
          counter++;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }else if (temp ==1) {   //if the up is pressed
          counter--;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }
        else{
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }
      }else if (counter == 2) {
        if (temp == 4) {     //if left is pressed(dexia)
          Minute_light_off_duration= Minute_light_off_duration + 1;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(">");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }
        else if(temp == 3) {     // if right is pressed(aristera)
          Minute_light_off_duration= Minute_light_off_duration - 1;
          if (Minute_light_off_duration <= 0 ) {
              Minute_light_off_duration=0;
          }
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(">");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }else if (temp==1) {   //if the up is pressed
          counter--;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }else if (temp==2) {   //if the down is pressed
          counter--;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }
        else{
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_light_off_duration);
          lcd.println(" Days ");
          lcd.print(Hour_light_off_duration);
          lcd.println(" Hours");
          lcd.print(">");
          lcd.print(Minute_light_off_duration);
          lcd.println(" Minute");
          lcd.display();
        }

      }
    lcd.clearDisplay();
    }

    //find the total minutes and set the array
    Total_Minute_light_off_duration = Day_light_off_duration * 24 * 60 + Hour_light_off_duration * 60 + Minute_light_off_duration;
    growthLightScheme[1] =Total_Minute_light_off_duration;
  }

void Set_water_on(MenuItem* p_menu_item)
{
    int counter=0;    //counts when the button is pressed
    int temp=0;
    while (temp <= 4) {
      lcd.setCursor(0, 1 * PCD8544_CHAR_HEIGHT);
      lcd.println("Water ON for: ");
      temp = readJoystick();
      if (counter == 0) {
        if (temp == 4) {    //if dexia is pressed(dexia)

          Day_water_on_duration= Day_water_on_duration + 1;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(">");
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }
        else if(temp == 3) {     // if right is pressed(aristera)
          Day_water_on_duration= Day_water_on_duration - 1;
          if (Day_water_on_duration <= 0 ) {
              Day_water_on_duration=0;
          }
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(">");
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }else if (temp==2) {   //if the down is pressed
          counter++;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
        }else if (temp==1) {   //if the up is pressed
          counter--;
          if (counter <= 0) {
            counter = 0;
          }
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
        }
        else{
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(">");
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }
      }else if (counter == 1) {
        if (temp == 4) {    //if left is pressed(dexia)
          Hour_water_on_duration= Hour_water_on_duration + 1;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }
        else if(temp == 3) {     // if right is pressed(aristera)
          Hour_water_on_duration= Hour_water_on_duration - 1;
          if (Hour_water_on_duration <= 0 ) {
              Hour_water_on_duration=0;
          }
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }else if (temp ==2) {   //if the down is pressed
          counter++;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }else if (temp ==1) {   //if the up is pressed
          counter--;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }
        else{
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(">");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }
      }else if (counter == 2) {
        if (temp == 4) {    //if left is pressed(dexia)
          Minute_water_on_duration= Minute_water_on_duration + 1;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(">");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }
        else if(temp == 3) {     // if right is pressed(aristera)
          Minute_water_on_duration= Minute_water_on_duration - 1;
          if (Minute_water_on_duration <= 0 ) {
              Minute_water_on_duration=0;
          }
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(">");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }else if (temp==1) {   //if the up is pressed
          counter--;
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }else if (temp==2) {   //if the down is pressed
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }
        else{
          lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
          lcd.print(Day_water_on_duration);
          lcd.println(" Days ");
          lcd.print(Hour_water_on_duration);
          lcd.println(" Hours");
          lcd.print(">");
          lcd.print(Minute_water_on_duration);
          lcd.println(" Minute");
          lcd.display();
        }

      }
    lcd.clearDisplay();
    }
    //find the total minutes and set the values4=
    Total_Minute_water_on_duration = Day_water_on_duration * 24 * 60 + Hour_water_on_duration * 60 +
        Minute_water_on_duration;
    irrigationScheme[0] =Total_Minute_water_on_duration;
  }

void Set_water_off(MenuItem* p_menu_item)
{
      int counter=0;    //counts when the button is pressed
      int temp=0;
      while (temp <= 4) {
        lcd.setCursor(0, 1 * PCD8544_CHAR_HEIGHT);
        lcd.println("Water OFF for: ");
        temp = readJoystick();
        if (counter == 0) {
          if (temp == 4) {    //if dexia is pressed(dexia)

            Day_water_off_duration= Day_water_off_duration + 1;
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(">");
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }
          else if(temp == 3) {     // if right is pressed(aristera)
            Day_water_off_duration= Day_water_off_duration - 1;
            if (Day_water_off_duration <= 0 ) {
                Day_water_off_duration=0;
            }
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(">");
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }else if (temp==2) {   //if the down is pressed
            counter++;
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(">");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
          }else if (temp==1) {   //if the up is pressed
            counter--;
            if (counter <= 0) {
              counter = 0;
            }
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(">");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
          }
          else{
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(">");
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }
        }else if (counter == 1) {
          if (temp == 4) {    //if left is pressed(dexia)
            Hour_water_off_duration= Hour_water_off_duration + 1;
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(">");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }
          else if(temp == 3) {     // if right is pressed(aristera)
            Hour_water_off_duration= Hour_water_off_duration - 1;
            if (Hour_water_off_duration <= 0 ) {
                Hour_water_off_duration=0;
            }
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(">");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }else if (temp ==2) {   //if the down is pressed
            counter++;
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(">");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }else if (temp ==1) {   //if the up is pressed
            counter--;
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(">");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }
          else{
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(">");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }
        }else if (counter == 2) {
          if (temp == 4) {    //if left is pressed(dexia)
            Minute_water_off_duration= Minute_water_off_duration + 1;
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(">");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }
          else if(temp == 3) {     // if right is pressed(aristera)
            Minute_water_off_duration= Minute_water_off_duration - 1;
            if (Minute_water_off_duration <= 0 ) {
                Minute_water_off_duration=0;
            }
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(">");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }else if (temp==1) {   //if the up is pressed
            counter--;
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }else if (temp==2) {   //if the down is pressed
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }else{
            lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
            lcd.print(Day_water_off_duration);
            lcd.println(" Days ");
            lcd.print(Hour_water_off_duration);
            lcd.println(" Hours");
            lcd.print(">");
            lcd.print(Minute_water_off_duration);
            lcd.println(" Minute");
            lcd.display();
          }

        }
      lcd.clearDisplay();
      }
      //find the total minutes and set the values4=
      Total_Minute_water_off_duration = Day_water_off_duration * 24 * 60 + Hour_water_off_duration * 60 +
          Minute_water_off_duration;
      irrigationScheme[0] =Total_Minute_water_off_duration;
    }

void Set_time(MenuItem* p_menu_item)
{
  bool buttonPressed= false;
  int counter = 0;
  int temp;
  while (buttonPressed == false) {
    lcd.clearDisplay();
    temp = readJoystick();
    if (temp == 5) {
      buttonPressed = true;
    }
    lcd.setCursor(0, 2 * PCD8544_CHAR_HEIGHT);
    lcd.println("(D/M/Y)");
    if(counter == 0){         //set the DAY
      lcd.setTextColor(WHITE, BLACK);
      if (temp == 1) {
        if (tm.Day < 31) {
          tm.Day = tm.Day +1 ;
        }
      }
      else if (temp == 2) {
        if (tm.Day > 0) {
          tm.Day = tm.Day -1 ;
        }
      }
      else if (temp == 3) {   //den paei piso
        /* code */
      }
      else if (temp == 4) {
        counter++;
      }
      lcd.setTextColor(WHITE, BLACK);
      lcd.print(tm.Day);
      lcd.setTextColor(BLACK, WHITE); //fix
      lcd.print("/");
      lcd.print(tm.Month);
      lcd.print("/");
      lcd.println(tmYearToCalendar(tm.Year));
      lcd.print("Time:");
      lcd.print(tm.Hour);
      lcd.print(":");
      lcd.print(tm.Minute);
      lcd.print(":");
      lcd.print(tm.Second);
      lcd.display();

    }
    else if(counter == 1){    //set the MONTH
      if (temp == 1) {
        if (tm.Month < 12) {
          tm.Month = tm.Month +1 ;
        }
      }
      else if (temp == 2) {
        if (tm.Month > 0) {
          tm.Month = tm.Month -1 ;
        }
      }
      else if (temp == 3) {   //den paei piso
        counter--;
      }
      else if (temp == 4) {
        counter++;
      }
      lcd.print(tm.Day);
      lcd.print("/");
      lcd.setTextColor(WHITE, BLACK);
      lcd.print(tm.Month);
      lcd.setTextColor(BLACK, WHITE); //fix
      lcd.print("/");
      lcd.println(tmYearToCalendar(tm.Year));
      lcd.print("Time:");
      lcd.print(tm.Hour);
      lcd.print(":");
      lcd.print(tm.Minute);
      lcd.print(":");
      lcd.print(tm.Second);
      lcd.display();


    }
    else if(counter == 2){    //set the YEAR
      if (temp == 1) {
          tm.Year= tm.Year +1 ;
      }
      else if (temp == 2) {
        if (tm.Year > 0) {
          tm.Year = tm.Year -1 ;
        }
      }
      else if (temp == 3) {   //den paei piso
        counter--;
      }
      else if (temp == 4) {
        counter++;
      }
      lcd.print(tm.Day);
      lcd.print("/");
      lcd.print(tm.Month);
      lcd.print("/");
      lcd.setTextColor(WHITE, BLACK);
      lcd.println(tmYearToCalendar(tm.Year));
      lcd.setTextColor(BLACK, WHITE); //fix
      lcd.print("Time:");
      lcd.print(tm.Hour);
      lcd.print(":");
      lcd.print(tm.Minute);
      lcd.print(":");
      lcd.print(tm.Second);
      lcd.display();


    }
    else if(counter == 3){    //set the Hour
      if (temp == 1) {
        if (tm.Hour < 24) {
          tm.Hour= tm.Hour +1 ;
        }
      }
      else if (temp == 2) {
        if (tm.Hour > 0) {
          tm.Hour = tm.Hour -1 ;
        }
      }
      else if (temp == 3) {   //den paei piso
        counter--;
      }
      else if (temp == 4) {
        counter++;
      }
      lcd.print(tm.Day);
      lcd.print("/");
      lcd.print(tm.Month);
      lcd.print("/");
      lcd.println(tmYearToCalendar(tm.Year));
      lcd.print("Time:");
      lcd.setTextColor(WHITE, BLACK);
      lcd.print(tm.Hour);
      lcd.setTextColor(BLACK, WHITE); //fix
      lcd.print(":");
      lcd.print(tm.Minute);
      lcd.print(":");
      lcd.print(tm.Second);
      lcd.display();
    }
    else if(counter == 4){    //set the MINUTE
      if (temp == 1) {
        if (tm.Minute < 60) {
          tm.Minute= tm.Minute +1 ;
        }
      }
      else if (temp == 2) {
        if (tm.Minute > 0) {
          tm.Minute = tm.Minute -1 ;
        }
      }
      else if (temp == 3) {   //den paei piso
        counter--;
      }
      else if (temp == 4) {
        counter++;
      }
      lcd.print(tm.Day);
      lcd.print("/");
      lcd.print(tm.Month);
      lcd.print("/");
      lcd.println(tmYearToCalendar(tm.Year));
      lcd.print("Time:");
      lcd.print(tm.Hour);
      lcd.print(":");
      lcd.setTextColor(WHITE, BLACK);
      lcd.print(tm.Minute);
      lcd.setTextColor(BLACK, WHITE); //fixs
      lcd.print(":");
      lcd.print(tm.Second);
      lcd.display();
    }
    else if(counter == 5){    //set the SECOND
      if (temp == 1) {
        if (tm.Second < 60) {
          tm.Second= tm.Second +1 ;
        }
      }
      else if (temp == 2) {
        if (tm.Second > 0) {
          tm.Second = tm.Second -1 ;
        }
      }
      else if (temp == 3) {   //den paei piso
        counter--;
      }
      else if (temp == 4) {
        //DO nothing it is the last
      }
      lcd.print(tm.Day);
      lcd.print("/");
      lcd.print(tm.Month);
      lcd.print("/");
      lcd.println(tmYearToCalendar(tm.Year));
      lcd.print("Time:");
      lcd.print(tm.Hour);
      lcd.print(":");
      lcd.print(tm.Minute);
      lcd.print(":");
      lcd.setTextColor(WHITE, BLACK);
      lcd.print(tm.Second);
      lcd.setTextColor(BLACK, WHITE); //fixs
      lcd.display();
    }
  }
  t = makeTime(tm);
 	    //use the time_t value to ensure correct weekday is set
  if(RTC.set(t) == 0) { // Success
      setTime(t);
      Serial << F("RTC set to: ");
      Serial << endl;
   }
}

// Menu variables
MenuSystem ms(my_renderer);
MenuItem mm_mi1("INFO", &info_selected);
Menu mu1("Light");
MenuItem mu1_1("Light ON for:", &Set_light_on);//prepei na fteia3o tis sinartiseis
MenuItem mu1_2("Light OFF for:", &Set_light_off);//
Menu mu2("Water");
MenuItem mu2_1("Water ON for:", &Set_water_on);//
MenuItem mu2_2("Water OFF for:", &Set_water_off);//
MenuItem mu3("Time", &Set_time);//


void serialPrintHelp()
{
  Serial.println("***************");
  Serial.println("w: go to previus item (up)");
  Serial.println("s: go to next item (down)");
  Serial.println("a: go back (right)");
  Serial.println("d: select \"selected\" item");
  Serial.println("?: print this help");
  Serial.println("h: print this help");
  Serial.println("***************");
}

void serialHandler() {
  int nav;
  nav = readJoystick();
    switch (nav) {
      case 1: // Previus item  m=1   up
        ms.prev();
        ms.display();
        break;
      case 2: // Next item   m=2     down
        ms.next();
        ms.display();
        break;
      case 3: // Back pressed   m=3 left
        ms.back();
        ms.display();
        break;
      case 5: // Select pressed press is m=5
        ms.select();
        ms.display();
        break;
      default:
        break;
    }
  nav=0;
  Alarm.delay(100);
}

void setup() {
  Serial.begin(9600);
  setupRTC();

  pinMode(PIN_LIGHT_1, OUTPUT);//set the growthLight
  digitalWrite(PIN_LIGHT_1, RELAYOFF);

  setTime(4,19,0,5,6,2015);//hr,min,sec,day,mnth,yr

    // Write current time into variable, because now() seems a lot of work behind the curtains
    timeNow = now();
    // Only continue if RTC is working correctly
    //if (timeStatus() == timeSet) {
    Serial.print(F("Time of startup: "));
    Serial.print(hour(timeNow));
    Serial.print(F(":"));
    Serial.print(minute(timeNow));

    Serial.print(F(" or "));
    Serial.println(timeNow);


    // Start the first calculation which light period we should be at now
    getGrowthLightPeriod();

    /* TIMER SETUP
        ========
    */

    // if growth light period is even, means growth light should be on
    // => turn on growth light
    if ((currentGrowthLightPeriod & 1) == 0) {
      turnOnGrowthLight();
    }
    // else if growth light period is odd
    // => set up next growth light ON event
    else {
      Alarm.timerOnce(secondsToNextGrowthLightSwitch, turnOnGrowthLight);
      Serial.print(F("Growth light ON in seconds: "));
      Serial.println(secondsToNextGrowthLightSwitch);
    }
    /*Start water*/
    turnOnIrrigation();

  //======joystick set up======
  pinMode (JoyStick_X, INPUT);
  pinMode (JoyStick_Y, INPUT);
  pinMode (JoyStick_Z, INPUT_PULLUP);
  //======End of joystick===
  lcd.begin();
  lcd.setContrast(60);

#ifdef HIDE_SPLASH
  lcd.clearDisplay();
#else
  lcd.display();
  Alarm.delay(2000);
#endif
  lcd.setTextSize(1);
  serialPrintHelp();

  ms.get_root_menu().add_item(&mm_mi1);
  ms.get_root_menu().add_menu(&mu1);
  mu1.add_item(&mu1_1);
  mu1.add_item(&mu1_2);
  ms.get_root_menu().add_menu(&mu2);
  mu2.add_item(&mu2_1);
  mu2.add_item(&mu2_2);
  ms.get_root_menu().add_item(&mu3);
  ms.display();
}

void loop(void)
{
  //GetTempHum(); //get the values from the sensor
  //GetSoilMu();
  readJoystick();     //read the value from the joystick
  serialHandler();    //handle the menu
  lcd.print("Cooling: ");
  Serial.print(F("Time: "));
  Serial.print(hour());
  Serial.print(F(":"));
  Serial.print(minute());
  Serial.print(F(":"));
  Serial.println(second());
}
