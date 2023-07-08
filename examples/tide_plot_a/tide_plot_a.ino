#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_ST7789.h>
#include "TidelibIndianRiverInletCoastGuardStation.h"     // https://tidesandcurrents.noaa.gov/noaatidepredictions.html?id=8558690
                             // 8570283 Ocean City Inlet, MD https://tidesandcurrents.noaa.gov/noaatidepredictions.html?id=8570283
                             // Lewis
RTC_DS3231 rtc;

#define TFT_CS        10
#define TFT_RST        9
#define TFT_DC         8
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

TideCalc myTideCalc;
float tideLevel;
DateTime levelTime;

int chartCols           = 240;                  // levels to store
int chartIntervalSecs   = 300;                  // time interval to plot
float chartLevel[280];                          // store tide levels

char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  // convert floats to fixed string
  // val      Your float variable;
  // width    Length of the string that will be created INCLUDING decimal point;
  // prec     Number of digits after the deimal point to print;
  // sout     Destination of output buffer
    char fmt[20];
    sprintf(fmt, "%%%d.%df", width, prec);
    sprintf(sout, fmt, val);
    return sout;
}



void setup() {
    Wire.begin();
    rtc.begin();
    Serial.begin(57600);

    if(rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    tft.init(240, 320);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLUE);
    tft.setTextWrap(false);

    getTides();
}

void loop() {
    DateTime now = rtc.now();
    char now_time_buf[] = "hh:mm:ss";
    char now_date_buf[] = "DDD MMMM DD, YYYY";
    
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
    tft.setTextSize(3);
    tft.setCursor(4, 24);

    tft.println(now.toString(now_time_buf));
    tft.println(now.toString(now_date_buf));

    delay(1000);

/*
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
*/
}

void getTides() {
    /**************************** Header ****************************/
    
    Serial.print("Tides at NOAA Station ");
    Serial.println(myTideCalc.returnStationIDnumber());
    Serial.print(chartIntervalSecs / 60);
    Serial.print("-min intervals levels over ");
    Serial.print(chartCols / 5);
    Serial.println(" hours");

    Serial.println(" Slot\t|   Time  \t\t| Tide  \t|  Rise\t| neg  \t| slope");
    Serial.println("     \t|   Stamp \t\t| Height\t| /Fall\t| Slope\t| Change");
    
    
    /****************************************************************/

    /************************* Initial Run **************************/
    
    bool negSlope, slopeChange;                                           // setup slope trackers
    char slope["⬇"]

    levelTime = rtc.now();                                                // get current time from RTC
    char level_time_buf[] = "MM/DD/YY hh:mm";                             // format time
    chartLevel[0] = myTideCalc.currentTide(levelTime);                    // get and store start tide level in slot 0
    Serial.print("0\t| ");                                                // print slot number
    Serial.print(levelTime.toString(level_time_buf));                     // print tide level timestamp
    Serial.print("\t| "); 
    Serial.print(chartLevel[0]);                                          // print tide level
    Serial.println(" ft.");

    /****************************************************************/

    levelTime = levelTime + TimeSpan(chartIntervalSecs);                  // increment time by interval

    if (myTideCalc.currentTide(levelTime) < chartLevel[0]) {
        slopeChange = true;
    }
    else slopeChange = false;

    /*************** loop though and store readings  ****************/

    for(int x = 1; x <= chartCols; x++ ) {
        char level_time_buf[] = "MM/DD/YY hh:mm";                         // format time
        chartLevel[x] = myTideCalc.currentTide(levelTime);                // get and store start tide level in slot x

        Serial.print(x);                                                  // print slot number
        Serial.print("\t| ");
        Serial.print(levelTime.toString(level_time_buf));                 // print tide level timestamp
        Serial.print("\t| ");
        Serial.print(chartLevel[x]);                                      // print tide level
        Serial.print(" ft.\t| ");

        // if (x > 1) {                                                      // if first time through skip up/down eval
            if (chartLevel[x] < chartLevel[x-1]) {                        // new level LESS THAN previous level
                negSlope = true;                                          // negative slope 
                Serial.print(" ⬇︎\t| ");                                   // print DOWN indicator
                Serial.print(negSlope);       Serial.print("\t| ");
                Serial.print(slopeChange);    Serial.print("\t| ");
                if (negSlope != slopeChange)  Serial.print("Low");       // on slope change print high 
            }
        // }
        // if (x > 1) {                                                      // if first time through skip up/down eval
            if (chartLevel[x] > chartLevel[x-1]) {                        // new level MORE THAN previous level
                negSlope = false;                                         // positive slope
                Serial.print(" ⬆︎\t| ");                                   // print UP indicator
                Serial.print(negSlope);       Serial.print("\t| ");
                Serial.print(slopeChange);    Serial.print("\t| "); 
                if (negSlope != slopeChange)  Serial.print("High");
            }
        // }

        slopeChange = negSlope;                                           // equalize the gates
        levelTime = levelTime + TimeSpan(chartIntervalSecs);              // increment time by interval
        Serial.println();                                                 // new line
    }
    
    /****************************************************************/
}
