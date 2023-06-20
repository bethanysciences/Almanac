 /*******************|*******************|*******************|*******************
 *  Almanac for specific Lattitudes and Longitudes
 *  Operates independantly not needing an Internet connection
 *  
 *  Bob Smith https://github.com/bethanysciences/almanac
 *  
 *  Time grabbed from Adafruit's battery backed I2C Maxim DS3231 basecReal Time 
 *  Clock (RTC) module (https://www.adafruit.com/product/3013) using Adafruit's 
 *  library (https://github.com/adafruit/RTClib) a fork of JeeLab's library 
 *  (https://git.jeelabs.org/jcw/rtclib) to Adafruit's Yelloow 1.2" 4-Digit 
 *  7-Segment I2C Backpack Display (https://www.adafruit.com/product/1269) driven
 *  by Adarfruit's LED Backpack https://github.com/adafruit/Adafruit_LED_Backpack
 *  and GFX  https://github.com/adafruit/Adafruit-GFX-Library libraries none
 *  included, so grab from inside of the Arduino IDE.
 *   
 *  Tide events calculated and pushed to high and low LED displays using Luke 
 *  Miller's  http://lukemiller.org library compiling NOAA harmonic data as a 
 *  derivative of David Flater's XTide application 
 *  https://flaterco.com/xtide/xtide.html see included Tide_calculator.md and 
 *  https://github.com/millerlp/Tide_calculator for more information and to 
 *  generate necessary location specific tables. Library for my spcific location 
 *  (Clearwater Beach, Florida, USA) included in this bundle.
 *  
 *  Sun events calculated and pushed to rise and set LED displays using DM 
 *  Kishi's library https://github.com/dmkishi/Dusk2Dawn, based on a port of 
 *  the NOAA Solar Calculator https://www.esrl.noaa.gov/gmd/grad/solcalc/
 *  
 *  Sun and Tide events written diymore based TM1637 0.56" LED displays 
 *  https://www.amazon.com/s?srs=20640512011 using Martin Prince's library 
 *  https://github.com/AKJ7/TM1637
 *  
 *  This application and project are open source using MIT License see 
 *  license.txt.
 *  
 *  See included library github directories for their respecive licenses
 *  
 *******************|*******************|*******************|******************/


#include <Wire.h>
#include "TM1637.h"
#include "RTClib.h"
#include "TidelibClearwaterBeachGulfOfMexicoFlorida.h"
#include "Dusk2Dawn.h"
#include "Adafruit_GFX.h"
#include "Adafruit_LEDBackpack.h"


/*******************|*******************|*******************|*******************
 *                           LOAD HARDWARE PROCESSES
 *******************|*******************|*******************|******************/

Adafruit_7segment clockDisplay = Adafruit_7segment();
#define DISPLAY_ADDRESS   0x77      // led backpack i2c address

int CLK  = 3;               // common clock line (white wire)
int DIO0 = 4;               // red sunrise display data line (green wire)
int DIO1 = 5;               // red sunrset display data line (green wire)
int DIO2 = 6;               // blue high tide display data line (green wire)
int DIO3 = 7;               // blue low tide display data line (green wire)

TM1637 riseDisplay(CLK, DIO0);  // instantiate sunrise display
TM1637 setDisplay(CLK, DIO1);   // instantiate sunrset display

TM1637 highDisplay(CLK, DIO2);  // instantiate high tide display
TM1637 lowDisplay(CLK, DIO3);   // instantiate low tide display

RTC_DS3231 rtc;                 // instantiate DS3231 Real Time Clock (RTC)


/*******************|*******************|*******************|*******************
 *                        LOAD GLOBAL LOCATION VARIABLES
 *******************|*******************|*******************|******************/

TideCalc tideLevel;
DateTime LOWTIDE, HIGHTIDE;

float LATITUDE    = 27.9719;
float LONGITUDE   = -82.8265;
int   GMTOFFSET   = -5;         // EDT
bool  HR12        = true;       // set true for 12 hour / false for 24 hour time
bool running = false;
int   NEXTSUNRISE, NEXTSUNSET;


/*******************|*******************|*******************|*******************
 *                            SETUP UPDATE INTERVALS
 *******************|*******************|*******************|******************/

const long clockInterval = 60000;           // 1-min clock update interval (ms)
unsigned long clockTicks = 0;               // store last loop time
const long tidesInterval = 3600000;         // 1-hour tides update interval (ms)
unsigned long tidesTicks = 0;               // store last loop time
const long setsInterval  = 3600000;         // interval update sets (ms)
unsigned long setsTicks  = 0;               // store last loop time

#define _DEBUG_                             // debug to serial port

void setup() {
#ifdef _DEBUG_
    Serial.begin(9600);
    Serial.println("Clock Start");
#endif
    highDisplay.init();                     // initialize displays
    lowDisplay.init();
    riseDisplay.init();
    setDisplay.init();

    clockDisplay.begin(DISPLAY_ADDRESS);    // initialize clock display

    if (! rtc.begin()) while (1);           // initialize RTC
    if (rtc.lostPower()) {          // reset to compile time if battery lost
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    delay(5000);

    writeClockDisplay();
    writeTidesDisplay();
    writeSetsDisplay();
}


/*******************|*******************|*******************|*******************
 *                                  MAIN LOOP
 *******************|*******************|*******************|******************/
 
void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - clockTicks >= clockInterval) {  // update clock
        clockTicks = currentMillis;
        writeClockDisplay();
    }
    if (currentMillis - tidesTicks >= tidesInterval) {  // update tide displays
        tidesTicks = currentMillis;
        writeTidesDisplay();
    }
    if (currentMillis - setsTicks >= setsInterval) {    // update sun displays
        setsTicks = currentMillis;
        writeSetsDisplay();
    }
}


/*******************|*******************|*******************|*******************
 *                     GET CURRENT TIME AND UPDATE DISPLAY
 *******************|*******************|*******************|******************/
 
void writeClockDisplay() {
    DateTime current = rtc.now();           // get current time from DS3231 RTC
    int hours = current.hour();
    if (HR12 && hours < 12) hours - 12;     // adjust hours for 12 hour time
    int minutes = current.minute();
    int displayValue = hours * 100 + minutes;   // load time into 4-digit number
    clockDisplay.print(displayValue, DEC);
    if (!HR12 && hours == 0) clockDisplay.writeDigitNum(1, 0);
    if (minutes < 10) clockDisplay.writeDigitNum(2, 0);
    clockDisplay.writeDisplay();                // push buffer to clock display

#ifdef _DEBUG_
    char buf0[60];
    sprintf(buf0, "%02d/%02d %02d:%02d",
            current.month(), current.day(), current.hour(), current.minute());
    Serial.println(buf0);
#endif
}


/*******************|*******************|*******************|*******************
 *                              DISPLAY SUN EVENTS
 *******************|*******************|*******************|******************/

void writeSetsDisplay() {
    getSets();                              // get / update sun events
    riseDisplay.dispNumber(NEXTSUNSET);     // shift to red display
    setDisplay.dispNumber(NEXTSUNRISE);     // shift to red display
}

/*******************|*******************|*******************|*******************
 *                             DISPLAY TIDE EVENTS
 *******************|*******************|*******************|******************/

void writeTidesDisplay() {
    getTides();                             // get / update tide events
    int highHour = HIGHTIDE.hour();
    int highMinute = HIGHTIDE.minute();
    int highTime = (highHour * 100) + highMinute;
    highDisplay.dispNumber(highTime);       // shift to blue display pin 6

    int lowHour = LOWTIDE.hour();
    int lowMinute = LOWTIDE.minute();
    int lowTime = (lowHour * 100) + lowMinute;
    lowDisplay.dispNumber(lowTime);         // shift to blue display pin 7
}


/*******************|*******************|*******************|*******************
 *                          CALCULATE NEXT TIDE EVENTES
 *******************|*******************|*******************|******************/

void getTides() {                                                           
    float baseHeight, lastHeight, nextHeight;
    bool  lastSlope,  nextSlope;
    char lowTide[40];
    char highTide[40];
    int rounds = 1;
    
    DateTime current = rtc.now();           // get current time from DS3231 RTC
    DateTime baseTime(current + TimeSpan(60));
    baseHeight = tideLevel.currentTide(baseTime);

    DateTime lastTime = baseTime;
    lastHeight = baseHeight;

    DateTime nextTime(lastTime + TimeSpan(60));
    nextHeight = tideLevel.currentTide(nextTime);

    if(lastHeight < nextHeight) nextSlope = 1;  // falling
    if(lastHeight > nextHeight) nextSlope = 0;  // rising

    lastSlope = nextSlope;
    
    while(rounds <= 2) {
        DateTime nextTime(lastTime + TimeSpan(60));
        nextHeight = tideLevel.currentTide(nextTime);
        if(lastHeight < nextHeight) nextSlope = 1;      // falling
        if(lastHeight > nextHeight) nextSlope = 0;      // rising
        if(nextSlope != lastSlope && lastSlope == 0) {  // low tide
            sprintf(lowTide, 
                "next LOW tide at %02d/%02d %02d:%02d level %.1f'", 
            lastTime.month(), lastTime.day(), lastTime.hour(), 
            lastTime.minute(), lastHeight);
            LOWTIDE = lastTime;
            // Serial.println(lowTide);             // for debugging
            rounds ++;
        }
        if(nextSlope != lastSlope && lastSlope == 1) {  // high tide
            sprintf(highTide, 
                "next HIGH tide at %02d/%02d %02d:%02d level %.1f'", 
            lastTime.month(), lastTime.day(), 
            lastTime.hour(), lastTime.minute(), lastHeight);
            HIGHTIDE = lastTime;
            // Serial.println(highTide);            // get next sun events
            rounds ++;
        }
        lastTime = nextTime;
        lastHeight = nextHeight;
        lastSlope = nextSlope;
    }

#ifdef _DEBUG_
    char buf2[60];
    sprintf(buf2, "High %02d%02d Low %02d%02d", 
            HIGHTIDE.hour(), HIGHTIDE.minute(), 
            LOWTIDE.hour(), LOWTIDE.minute());
    Serial.println(buf2);
#endif
}


/*******************|*******************|*******************|*******************
 *                           CALCULATE NEXT SUN EVENTES
 *******************|*******************|*******************|******************/
 
void getSets() {                        // get next sun events
    DateTime now = rtc.now();           // get current time from DS3231 RTC
    float floatHour, floatMinute;
    int byteHour, byteMinute;
    Dusk2Dawn ClearwaterBeach(LATITUDE, LONGITUDE, GMTOFFSET);
    
    int nextSunriseSinceMidnight = 
        ClearwaterBeach.sunrise(now.year(), now.month(), now.day(), true);
    floatHour   = nextSunriseSinceMidnight / 60.0;
    floatMinute = 60.0 * (floatHour - floor(floatHour));
    byteHour    = (int) floatHour;
    byteMinute  = (int) floatMinute;
    if (byteMinute > 59) {
        byteHour   += 1;
        byteMinute  = 0;
    }
    NEXTSUNRISE = (byteHour * 100) + byteMinute;
      
    int nextSunsetSinceMidnight = 
        ClearwaterBeach.sunset(now.year(), now.month(), now.day(), true);
    floatHour   = nextSunsetSinceMidnight / 60.0;
    floatMinute = 60.0 * (floatHour - floor(floatHour));
    byteHour    = (int) floatHour;
    byteMinute  = (int) floatMinute;
    if (byteMinute > 59) {
        byteHour   += 1;
        byteMinute  = 0;
    }
    NEXTSUNSET = (byteHour * 100) + byteMinute;

#ifdef _DEBUG_
    char buf1[80];
    sprintf(buf1, "Rise %d Set %d  ", NEXTSUNRISE, NEXTSUNSET);
    Serial.println(buf1);
#endif
}
