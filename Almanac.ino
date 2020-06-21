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
 *  Output to serial monitor for test and tweaking see example folder for
 *  versions driving seven-segment leds and oled displays.
 *  
 *  This application and project are open source using MIT License see 
 *  license.txt.
 *  
 *  See included library github directories for their respecive licenses
 *  
 *******************|*******************|*******************|******************/

#include <Arduino.h>
#include <RTClib.h>                         // version=1.8.0
#include <SunEvent.h>                       // version=0.1.0
#include "TidelibClearwaterBeachGulfOfMexicoFlorida.h"


/*******************|*******************|*******************|*******************
 *                 LOAD GLOBAL HARDWARE VARIABLES & FUNCTIONS
 *******************|*******************|*******************|******************/

RTC_DS3231 rtc;                             // instantiate RTC module
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
float INTERVAL              = 1 * 5 * 60L;  // tide calc interval
float results;                              // needed to print tide height


/*******************|*******************|*******************|*******************
 *                                    SETUP 
 *******************|*******************|*******************|******************/

void setup() {
    asm(".global _printf_float");           // printf renders floats

    Serial.begin(115200);
    //while (!Serial);                        // wait for serrial port to open
    
    if (! rtc.begin()) while (1);           // wait until RTC starts
    if (rtc.lostPower()) {                  // if module lost power reset time
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    delay(5000);                            // flush registers
}

void loop() {

    /******************|*******************|*******************|****************
    *                     GET CURRENT TIME AND UPDATE DISPLAY
    *******************|*******************|*******************|***************/
    
    DateTime now = rtc.now();                   // get current time from RTC
    char time_buf[] = "Current time MM/DD/YY hh:mm:ss ap ";
    Serial.print(now.toString(time_buf));
    Serial.println(LOC[isLDT(now)]);            // render LST ot LDT


    /******************|*******************|*******************|****************
    *                         RUN SUN AND TIDE EVENTS 
    *******************|*******************|*******************|***************/
    
    getTideEvents();
    getSunEvents();

    delay(20000);
    
    Serial.println();
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

                char high_buf[] = "Next High tide MM/DD/YY hh:mm ap ";
                Serial.print(adjHigh.toString(high_buf));    
                Serial.print(LOC[isLDT(adjHigh)]);            // render LST ot LDT
                Serial.print(" level ");                
                Serial.print(resultsHigh, 2);
                Serial.println(" feet");  


                
                DateTime adjLow(futureLow.unixtime() + 
                         (isLDT(futureLow) * 3600));
                
                char low_buf[] = "Next Low tide  MM/DD/YY hh:mm ap ";
                Serial.print(adjLow.toString(low_buf));    
                Serial.print(LOC[isLDT(adjLow)]);            // render LST ot LDT
                Serial.print(" level ");                
                Serial.print(resultsLow, 2);
                Serial.println(" feet");
                
            }
            else {
                DateTime adjLow(futureLow.unixtime() + 
                         (isLDT(futureLow) * 3600));
                
                char low_buf[] = "Next Low tide  MM/DD/YY hh:mm ap ";
                Serial.print(adjLow.toString(low_buf));    
                Serial.print(LOC[isLDT(adjLow)]);            // render LST ot LDT
                Serial.print(" level ");                
                Serial.print(resultsLow, 2);
                Serial.println(" feet");
                
                DateTime adjHigh(futureHigh.unixtime() + 
                         (isLDT(futureHigh) * 3600)); 
                
                char high_buf[] = "Next High tide MM/DD/YY hh:mm ap ";
                Serial.print(adjHigh.toString(high_buf));    
                Serial.print(LOC[isLDT(adjHigh)]);            // render LST ot LDT
                Serial.print(" level ");                
                Serial.print(resultsHigh, 2);
                Serial.println(" feet");  
            
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
 *                         CALC AND DISPLAY SUN EVENTS
 *******************|*******************|*******************|******************/
 
void getSunEvents() {
    DateTime nextrise = ClearwaterBeach.sunrise(rtc.now());    
    char rise_buf[] = "Next Sunrise hh:mm ap ";
    Serial.print(nextrise.toString(rise_buf));
    Serial.println(LOC[isLDT(nextrise)]);               // render LST ot LDT

    DateTime nextset  = ClearwaterBeach.sunset(rtc.now());    
    char set_buf[] = "Next Sunset hh:mm ap ";
    Serial.print(nextset.toString(set_buf));
    Serial.println(LOC[isLDT(nextset)]);                // render LST ot LDT
    
    TimeSpan sunlight = nextset.unixtime() - nextrise.unixtime();
    char light_buf[20];
    sprintf(light_buf, "%d hours %d minutes of daylight today", 
            sunlight.hours(), sunlight.minutes());
    Serial.println(light_buf);
}


/*******************|*******************|*******************|*******************
 *                    DETERMINE IF US DAYLIGHT SAVINGS TIME
 *******************|*******************|*******************|******************/
 
bool isLDT(const DateTime& curLST) {
    if(!USE_USLDT) return false;            // location uses US daylight savings
    int y = curLST.year() - 2000;           
    int x = (y + y/4 + 2) % 7;              // set boundary Sundays

    if(curLST.month() == 3 && 
       curLST.day() == (14 - x) && 
       curLST.hour() >= 2)
       return true;                         // time is Local Daylight Time (LDT)

    if(curLST.month() == 3 && 
       curLST.day() > (14 - x) || 
       curLST.month() > 3) 
       return true;                         // time is Local Daylight Time (LDT)

    if(curLST.month() == 11 && 
       curLST.day() == (7 - x) && 
       curLST.hour() >= 2) 
       return false;                        // time is Local Standard Time (LST)
   
    if(curLST.month() == 11 &&
       curLST.day() > (7 - x) || 
       curLST.month() > 11 || 
       curLST.month() < 3) 
       return false;                        // time is Local Standard Time (LST)
}