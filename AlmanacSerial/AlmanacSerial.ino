#include "RTClib.h"
// RTC_DS3231 rtc;                                         // use to instantiate DS3231 RTC module
RTC_Millis rtc;                                            // use w/o clock module

bool isLDT();
char LSD[3][4]      = {"LST","LDT"};

// ------------  SUN EVENTS  ---------------------- //
#include <Dusk2Dawn.h>
Dusk2Dawn clearwater(27.978, -82.832, -5);                 // Station CWBF1 - 8726724 - Clearwater Beach, FL

// ------------  TIDE CALCULATIONS  --------------- //
// #include "TidelibDelawareCityDelawareRiverDelaware.h"
#include "TidelibClearwaterBeachGulfOfMexicoFlorida.h"
TideCalc myTideCalc;

void setup() {
    asm(".global _printf_float");
    Serial.begin(57600);

    rtc.begin(DateTime(2023, 3, 12, 1, 59, 53));           // Start LDT 2nd Sunday Mar 02:00 3/12/23
    // rtc.begin(DateTime(2023, 11, 5, 1, 59, 55));        // End LDT 1st Sunday Nov 02:00 11/5/23

    delay(5000);
    Serial.println("------------------------ START ------------------------");

    displayTime();
    getTideEvents();
    getSunEvents();
    
    for (int i = 1; i <= 5; i++) {
        displayTime();
        delay(1000);
    }  
}

void loop() {
}

bool isLDT(const DateTime& curLST) {
    if ((curLST.month()  >  3 && curLST.month() < 11 ) || 
        (curLST.month() ==  3 && curLST.day() >= 8 && curLST.dayOfTheWeek() == 0 && curLST.hour() >= 2) ||
        (curLST.month() == 11 && curLST.day() <  8 && curLST.dayOfTheWeek() >  0) || 
        (curLST.month() == 11 && curLST.day() <  8 && curLST.dayOfTheWeek() == 0 && curLST.hour() < 2)) {
        return true;
        }
    else {
        return false;
    }
}

void displayTime() {
    DateTime now = rtc.now();
    DateTime adjtime(now.unixtime() + (isLDT(rtc.now()) * 3600));

    char curr_adjtime_buf[] = "hh:mm:ss DDD MMM DD, YYYY ";    
    Serial.print("Adjusted time ");
    Serial.print(adjtime.toString(curr_adjtime_buf));
    Serial.print(LSD[isLDT(adjtime)]);

    Serial.print(" | Time stored in RTC ");
    char curr_time_buf[] = "hh:mm:ss DDD MMM DD, YYYY ";
    Serial.print(now.toString(curr_time_buf));
    Serial.println("LST");
}

void getTideEvents() {
    float INTERVAL              = 1 * 5 * 60L;      // tide calc interval
    float results;                                  // needed to print tide height
    DateTime futureHigh;
    DateTime futureLow;
    DateTime future;
    uint8_t slope;
    uint8_t i                   = 0;
    uint8_t zag                 = 0;
    bool    gate                = 1;
    float   tidalDifference     = 0;
    bool    bing                = 1;
    bool    futureLowGate       = 0;
    bool    futureHighGate      = 0;

    DateTime now = rtc.now();                                       // current time from RTC
    DateTime adjnow(now.unixtime() - (isLDT(rtc.now()) * 3600));    // convert to LST during LDT

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
                DateTime adjHigh(futureHigh.unixtime() + (isLDT(futureHigh) * 3600));

                char high_buf[] = "Next High tide MM/DD/YY hh:mm ";
                Serial.print(adjHigh.toString(high_buf));
                Serial.print(LSD[isLDT(adjHigh)]);            // render LST or LDT
                Serial.print(" level ");
                Serial.print(resultsHigh, 2);
                Serial.print(" feet");

                DateTime adjLow(futureLow.unixtime() + (isLDT(futureLow) * 3600));

                char low_buf[] = " | Next Low tide MM/DD/YY hh:mm ";
                Serial.print(adjLow.toString(low_buf));
                Serial.print(LSD[isLDT(adjLow)]);            // render LST or LDT
                Serial.print(" level ");
                Serial.print(resultsLow, 2);
                Serial.print(" feet | ");

            }
            else {
                DateTime adjLow(futureLow.unixtime() + 
                         (isLDT(futureLow) * 3600));

                char low_buf[] = "Next Low tide MM/DD/YY hh:mm ";
                Serial.print(adjLow.toString(low_buf));
                Serial.print(LSD[isLDT(adjLow)]);            // render LST or LDT
                Serial.print(" level ");
                Serial.print(resultsLow, 2);
                Serial.print(" feet");

                DateTime adjHigh(futureHigh.unixtime() + 
                         (isLDT(futureHigh) * 3600));

                char high_buf[] = " | Next High tide MM/DD/YY hh:mm ";
                Serial.print(adjHigh.toString(high_buf));
                Serial.print(LSD[isLDT(adjHigh)]);            // render LST or LDT
                Serial.print(" level ");
                Serial.print(resultsHigh, 2);
                Serial.print(" feet | ");

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

void getSunEvents() {
    DateTime now = rtc.now();
    int Sunrise = clearwater.sunrise(now.year(), now.month(), now.day(), isLDT(now));  // returns mins past midnight
    char risebuff[5];
    Dusk2Dawn::min2str(risebuff, Sunrise);
    Serial.print("Sunrise "); Serial.print(risebuff);

    int Sunset = clearwater.sunset(now.year(), now.month(), now.day(), isLDT(now));  // returns mins past midnight
    char setbuff[5];
    Dusk2Dawn::min2str(setbuff, Sunset);
    Serial.print(" | Sunset "); Serial.println(setbuff);
}
