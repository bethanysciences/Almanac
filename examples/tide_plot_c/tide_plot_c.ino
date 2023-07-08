#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_ST7789.h>
#include "TidelibIndianRiverInlet(CoastGuardStation).h"

RTC_DS3231 rtc;

#define TFT_CS        10
#define TFT_RST        9
#define TFT_DC         8
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

TideCalc myTideCalc;
float tideLevel;
DateTime levelTime;

int chartCols = 288;                        // 288 chart columns
int chartIntervalSecs  = 300;               // time interval to plot
float chartLevels[288];                     // store tide leves

void setup() {
    Wire.begin();
    rtc.begin();
    Serial.begin(57600);

    if(rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    tft.init(240, 320);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLUE);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
    tft.setTextSize(2);
    tft.setTextWrap(false);

    DateTime now = rtc.now();
    char now_time_buf[] = "MM/DD/YYYY hh:mm:ss";
    
    levelTime = DateTime(now);
    tft.setCursor(4, 220);
    tft.println(now.toString(now_time_buf));
    
    // Serial.print(myTideCalc.returnStationID());
    // Serial.print("NOAA Station ");
    // Serial.println(myTideCalc.returnStationIDnumber());
    Serial.println("Tide levels 5-min increments next 24-hours");
    Serial.println(now.toString(now_time_buf));
    Serial.println("Column \t|      Level Time  \t|      Tide height");

    for(int x = 1; x <= chartCols; x++ ) {
        char level_time_buf[] = "MM/DD/YYYY hh:mm:ss";
        tideLevel = myTideCalc.currentTide(levelTime);
        // chartLevel[x] = tideLevel;
        Serial.print(x);                                  
        Serial.print("\t|  ");
        Serial.print(levelTime.toString(level_time_buf)); 
        Serial.print("\t|  ");
        Serial.print(tideLevel);
        Serial.print(" ft.");
        Serial.println();

        levelTime = levelTime + TimeSpan(chartIntervalSecs);
    }
}

void loop() {
}
