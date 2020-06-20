 /*******************|*******************|*******************|*******************
 *  Almanac for specific Latitude and Longitudes
 *  Operates independantly not needing an Internet connection
 *  
 *  Bob Smith https://github.com/bethanysciences/almanac
 *  
 *  Written under version 1.8.12 of the Arduino IDE
 *  for Arduino Nano 33 IoT and Arduino Nano 33 BLE microprocessors
 *  
 *  This program distributed WITHOUT ANY WARRANTY; without even the implied 
 *  warranty of MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  
 *  Predictions generated by this program should NOT be used for navigation.
 *  No accuracy or warranty is given or implied for these predictions.
 *  
 *  Time grabbed from Adafruit's battery backed I2C Maxim DS3231 Real Time 
 *  Clock (RTC) module (https://www.adafruit.com/product/3013) using Adafruit's 
 *  library (https://github.com/adafruit/RTClib) a fork of JeeLab's library 
 *  (https://git.jeelabs.org/jcw/rtclib). - install from Arduino IDE as [RTClib]
 *   
 *  Tide events calculated using Luke Miller's library of NOAA harmonic data 
 *  https://github.com/millerlp/Tide_calculator derived from David Flater's
 *  XTide application https://flaterco.com/xtide/xtide.html. Futher logic is
 *  derived from rabbitcreek's https://github.com/rabbitcreek/tinytideclock
 *  
 *  Sun events calculated using SunEvents library 
 *  https://github.com/bethanysciences/SunEvents outputting sunrises and sets
 *  to a RTClib DateTime object, a aerivative of DM Kishi's library 
 *  https://github.com/dmkishi/Dusk2Dawn, based on a port of NOAA's Solar 
 *  Calculator https://www.esrl.noaa.gov/gmd/grad/solcalc/
 *  
 *  Clock output to Adafruit's 1.2" 4-Digit 7-Segment Display w/I2C Backpack 
 *  - Yellow https://www.adafruit.com/product/1269 driven by Adafruit's LED
 *  Backpack https://github.com/adafruit/Adafruit_LED_Backpack and
 *  GFX https://github.com/adafruit/Adafruit-GFX-Librarylibraries libraries  
 *  - install both from Arduino IDE as [Adafruit LED Backpack Library] and
 *  [Adafruit GFX Library]
 *  
 *  Tide and Sun events output to Diymore red (sun) and blue (tides) TM1637 
 *  0.56" 7 Segment 4 Digit LED driven by Martin Prince's 
 *  https://github.com/AKJ7/TM1637 library - install from Arduino IDE as 
 *  [TM1637_Driver]
 *  
 *  This application and project are open source using MIT License see 
 *  license.txt.
 *  
 *  See included library github directories for their respecive licenses
 *  
 *******************|*******************|*******************|******************/


#include <RTClib.h>                         // version=1.8.0 
#include <SunEvent.h>                       // version=0.1.0
#include <Adafruit_GFX.h>                   // version=1.9.0  
#include <Adafruit_LEDBackpack.h>           // version=1.1.7
#include <TM1637.h>                         // version=1.1.2
#include "TidelibClearwaterBeachGulfOfMexicoFlorida.h"


/*******************|*******************|*******************|*******************
 *                 LOAD GLOBAL HARDWARE VARIABLES & FUNCTIONS
 *******************|*******************|*******************|******************/

Adafruit_7segment clockDisplay = Adafruit_7segment();
#define DISPLAY_ADDRESS   0x75      // led backpack i2c address

int CLK  = 3;               // common clock line (white wire)
int DIO0 = 4;               // red sunrise display data line (green wire)
int DIO1 = 5;               // red sunrset display data line (green wire)
int DIO2 = 6;               // blue high tide display data line (green wire)
int DIO3 = 7;               // blue low tide display data line (green wire)

TM1637 riseDisplay(CLK, DIO0);  // instantiate sunrise display
TM1637 setDisplay(CLK, DIO1);   // instantiate sunrset display

TM1637 highDisplay(CLK, DIO2);  // instantiate high tide display
TM1637 lowDisplay(CLK, DIO3);   // instantiate low tide display

RTC_DS3231 rtc;
#define USE_USLDT           true            // location uses US daylight savings
char LOC[3][4]              = {"LST",       // for Local Standard Time (LST)
                               "LDT"};      // and Local Daylight Time (LDT)
#define GMT_OFFSET          -5              // LST hours GMT offset


/*******************|*******************|*******************|*******************
 *                  LOAD GLOBAL LOCATION VARIABLES & FUNCTIONS
 *******************|*******************|*******************|******************/

float LATITUDE              = 27.9719;
float LONGITUDE             = -82.8265;
SunEvent ClearwaterBeach(LATITUDE, LONGITUDE, GMT_OFFSET);

TideCalc myTideCalc;
float INTERVAL              = 1 * 5 * 60L;              // tide calc interval
float results;


/*******************|*******************|*******************|*******************
 *                                    SETUP 
 *******************|*******************|*******************|******************/

void setup() {
`1   
    highDisplay.init();                     // initialize event displays
    lowDisplay.init();
    riseDisplay.init();
    setDisplay.init();

    clockDisplay.begin(DISPLAY_ADDRESS);    // initialize clock display
    
    if (! rtc.begin()) while (1);           // initialize Real Time Clock (RTC)
    if (rtc.lostPower()) {                  // if module lost power reset time
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    delay(5000);                            // flush registers
}   


void loop() {

    /******************|*******************|*******************|**************
    *                     GET CURRENT TIME AND UPDATE DISPLAY
    *******************|*******************|*******************|*************/
 
    DateTime now = rtc.now();                   // get current time from RTC
    clockDisplay.print((now.twelveHour() * 100) + 
                         now.minute());
    clockDisplay.writeDisplay();            // push buffer to clock display


    /******************|*******************|*******************|**************
    *                         RUN SUN AND TIDE EVENTS 
     *******************|*******************|*******************|*************/

    getTideEvents();
    getSunEvents();
    
    for(int x = 1; x <= 30; x++) {          // flas colon as activity sprite
        clockDisplay.drawColon(true);
        clockDisplay.writeDisplay();
        delay(1000);
        clockDisplay.drawColon(false);
        clockDisplay.writeDisplay();
        delay(1000);
    }
}


/*******************|*******************|*******************|*******************
 *                         CALC AND DISPLAY TIDE EVENTS
 *******************|*******************|*******************|******************/

void getTideEvents() {
    
    DateTime futureHigh;
    DateTime futureLow;
    DateTime future;
    uint8_t slope;
    uint8_t i               = 0;
    uint8_t zag             = 0;
    bool    gate            = 1;
    float   tidalDifference = 0;
    bool    bing            = 1;
    bool    futureLowGate   = 0;
    bool    futureHighGate  = 0;
    
    DateTime now = rtc.now();                   // current time from RTC

    DateTime adjnow(now.unixtime() 
             - (isLDT(now) * 3600));            // convert to LST during LDT
             
    float pastResult = myTideCalc.currentTide(adjnow);
        
    while(bing){
        i++;
        DateTime future(adjnow.unixtime() + (i * INTERVAL));
        results = myTideCalc.currentTide(future);
        tidalDifference = results - pastResult;
        if (gate){
            if(tidalDifference < 0) slope = 0;
            else slope = 1;
            gate = 0;
        }
        if(tidalDifference > 0 && slope == 0) {
            futureLow = future;
            gate = 1;
            //bing = 0;
            futureLowGate = 1;
        }
        else if(tidalDifference < 0 && slope == 1){
            futureHigh = future;
            gate = 1;
            //bing = 0;
            futureHighGate = 1;
        }
        if(futureHighGate && futureLowGate) {
            bool hiLow;
            float resultsHigh = myTideCalc.currentTide(futureHigh);
            float resultsLow  = myTideCalc.currentTide(futureLow);
        
            if(int(futureHigh.unixtime() - futureLow.unixtime()) < 0) hiLow = 1;
            if(int(futureHigh.unixtime() - futureLow.unixtime()) > 0) hiLow = 0;
            
            if (hiLow) {
                DateTime adjHigh(futureHigh.unixtime() + 
                         (isLDT(futureHigh) * 3600));
                
                highDisplay.dispNumber((adjHigh.hour() * 100) +
                           adjHigh.minute());
                

                DateTime adjLow(futureLow.unixtime() + 
                         (isLDT(futureLow) * 3600));
                
                lowDisplay.dispNumber((adjLow.hour() * 100) +
                           adjLow.minute());
                
            }
            else {
                DateTime adjLow(futureLow.unixtime() + 
                         (isLDT(futureLow) * 3600));
               
                lowDisplay.dispNumber((adjLow.hour() * 100) +
                                       adjLow.minute());


                DateTime adjHigh(futureHigh.unixtime() + 
                         (isLDT(futureHigh) * 3600));
                
                highDisplay.dispNumber((adjHigh.hour() * 100) +
                           adjHigh.minute());

                //Serial.println((adjHigh.hour() * 100) +
                //                adjHigh.minute());
            }
            results = myTideCalc.currentTide(adjnow);
            gate = 1;
            bing = 0;
            futureHighGate = 0;
            futureLowGate  = 0;
        }
        pastResult = results;
    }
}


/*******************|*******************|*******************|*******************
 *                           CALC AND DISPLAY SUN EVENTS
 *******************|*******************|*******************|******************/

void getSunEvents() {   
    DateTime nextrise = ClearwaterBeach.sunrise(rtc.now());
    riseDisplay.display((nextrise.hour() * 100) +
                           nextrise.minute());

    DateTime nextset  = ClearwaterBeach.sunset(rtc.now());    
    setDisplay.display((nextset.hour() * 100) +
                           nextset.minute());
}


/*******************|*******************|*******************|*******************
 *                    DETERMINE IF US DAYLIGHT SAVINGS TIME
 *******************|*******************|*******************|******************/
 
bool isLDT(const DateTime& curLST) {
   if(!USE_USLDT) return false;             // location uses US daylight savings
   int y = curLST.year() - 2000;
   int x = (y + y/4 + 2) % 7;               // find boundary Sundays

   if(curLST.month() == 3 && 
      curLST.day() == (14 - x) && 
      curLST.hour() >= 2)
      return true;                          // time is Local Daylight Time (LDT)
   
   if(curLST.month() == 3 && 
      curLST.day() > (14 - x) || 
      curLST.month() > 3) 
      return true;                          // time is Local Daylight Time (LDT)
   
   if(curLST.month() == 11 && 
      curLST.day() == (7 - x) && 
      curLST.hour() >= 2) 
      return false;                         // time is Local Standard Time (LST)
   
   if(curLST.month() == 11 && 
      curLST.day() > (7 - x) || 
      curLST.month() > 11 || 
      curLST.month() < 3) 
      return false;                         // time is Local Standard Time (LST)
}
