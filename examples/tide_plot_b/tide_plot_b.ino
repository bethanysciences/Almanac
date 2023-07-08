#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "TidelibIndianRiverInletCoastGuardStation.h"     // https://tidesandcurrents.noaa.gov/noaatidepredictions.html?id=8558690
// 8570283 Ocean City Inlet, MD

RTC_DS3231 rtc;

#define TFT_CS        10
#define TFT_RST        9
#define TFT_DC         8
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

TideCalc myTideCalc;
float tideLevel;
DateTime levelTime;

int chartIntervalSecs  = 300;             // 2-min intervals
int chartCols = 280;                      // 18hrs @ 1min ea levels to store
float chartLevel[280];                    // storage array
uint16_t startY       = 100;
uint16_t startX       = 10;
uint16_t graphYFactor = 20;
bool gate, prevGate;
uint16_t l = 0;
uint16_t h = 0;

void setup() {
    Wire.begin();
    rtc.begin();
    Serial.begin(57600);

    if(rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    tft.init(240, 320);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLUE);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
    tft.setTextWrap(false);

    getTides();
}

void loop() {
    DateTime now = rtc.now();
    char now_time_buf[] = "MM/DD/YYYY hh:mm:ss";
    levelTime = DateTime(now);
    tft.setTextSize(2);
    tft.setCursor(0, 24);
    tft.println(now.toString(now_time_buf));
    delay(1000);
}

void getTides() {
    tft.setTextSize(2);
    tft.setCursor(0, 5);
    tft.print("USCG Sta Indian River Inlet");

    DateTime now = rtc.now();
    char now_time_buf[] = "MM/DD/YYYY hh:mm:ss";
    levelTime = DateTime(now);
    chartLevel[0] = myTideCalc.currentTide(levelTime);
    tft.drawPixel(startX, startY + (chartLevel[0] * graphYFactor), ST77XX_WHITE);
    levelTime = levelTime + TimeSpan(chartIntervalSecs);

    for(int x = 1; x <= chartCols; x++ ) {
        char level_time_buf[] = "MM/DD hh:mm";
        chartLevel[x] = myTideCalc.currentTide(levelTime);
        tft.drawPixel(startX + x, startY + (chartLevel[x] * graphYFactor), ST77XX_WHITE);
        
        if (x > 1) {
            if (chartLevel[x] <= chartLevel[x-1]) {
                gate = false;
                if (gate != prevGate) {
                    tft.setTextSize(1);
                    tft.setCursor(h, 75);
                    tft.print(levelTime.toString(level_time_buf));   tft.print(" ");
                    tft.print(chartLevel[x-1]);
                    tft.println(" H");
                    h =+ 20;
                }
            }
        }
        if (x > 1) {
            if (chartLevel[x] > chartLevel[x-1]) {
                gate = true;
                if (gate != prevGate) {
                    tft.setTextSize(1);
                    tft.setCursor(l, 200);
                    tft.print(levelTime.toString(level_time_buf));    tft.print(" ");
                    tft.print(chartLevel[x-1]);
                    tft.println(" L");
                    l =+ 20;
                }
            }
        }
        prevGate = gate;
        levelTime = levelTime + TimeSpan(chartIntervalSecs);
    }
}