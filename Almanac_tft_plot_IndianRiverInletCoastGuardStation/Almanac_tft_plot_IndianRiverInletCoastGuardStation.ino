#include "RTClib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "TidelibIndianRiverInletCoastGuardStation.h"
#include "Dusk2Dawn.h"

RTC_DS3231 rtc;                                 // instantiate DS3231 Real Time Clock (RTC)

#define TFT_CS        10
#define TFT_RST        9
#define TFT_DC         8
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

TideCalc tideLevel;
DateTime LowTide, HighTide;

float LATITUDE   = 38.609;
float LONGITUDE  = -75.062;
float GMT_OFFSET = -5;
bool DST = true;
Dusk2Dawn IndianRiverInlet(LATITUDE, LONGITUDE, GMT_OFFSET);

const long clockInterval  = 60000;               // 1-min - clock update interval (ms)
const long tideInterval   = 3600000;             // 1-hour - tides update interval (ms)
const long sunInterval    = 3600000;             // 1-hour - sets update interval (ms)

uint16_t line_DateTime    = 5;
uint16_t line_SunEvents   = 25;
uint16_t line_HighTide    = 65;
uint16_t line_LowTide     = 222;

void setup() {
    Wire.begin();
    Serial.begin(57600);
    rtc.begin();
    if(rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    delay(50);

    tft.init(240, 320);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLUE);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLUE);
    tft.setTextWrap(false);

    Serial.println("*******************************************************");
    displayGrid();
    displayDateTime();
    displayTideEvents();
    getSunEvents();
}

void loop() {
    unsigned long clockTicks, tideTicks, sunTicks;
    unsigned long currentMillis = millis();
    if (currentMillis - clockTicks >= clockInterval) {  // update clock
        clockTicks = currentMillis;
        displayDateTime();
    }
    if (currentMillis - tideTicks >= tideInterval) {    // update tide displays
        tideTicks = currentMillis;
        displayTideEvents();
    }
    if (currentMillis - sunTicks >= sunInterval) {      // update sun displays
        sunTicks = currentMillis;
        getSunEvents();
    }
}

void displayGrid(){
    tft.fillRect(0, 85, 320, 125, ST77XX_YELLOW);
    tft.drawFastHLine(0, 115, 320, ST77XX_BLACK);
    tft.drawFastHLine(0, 145, 320, ST77XX_BLACK);
    tft.drawFastHLine(0, 175, 320, ST77XX_BLACK);
}

void displayDateTime() {
    DateTime now = rtc.now();                           // get current time from DS3231 RTC
    char now_datetime_buf[] = "DDD MM/DD/YY hh:mm:ss"; 
    tft.setTextSize(2);
    tft.setCursor(0, line_DateTime);
    tft.println(now.toString(now_datetime_buf));
    // Serial.println(now.toString(now_datetime_buf));
}

void displayTideEvents() {      
    double x_plot       = 1;
    double x_factor     = .5;
    double y_baseLine   = 100;
    double y_factor     = 30;   
    int rounds          = 1;                                                   
    
    DateTime levelTime = rtc.now();                       // get current time from DS3231 RTC
    float level = tideLevel.currentTide(levelTime);

    DateTime nextTime(levelTime + TimeSpan(60));          // add 60 seconds
    float nextLevel = tideLevel.currentTide(nextTime);

    char level_d[4];
    char Tide_buf[25];
    dtostrf(level, 4, 2, level_d);
    sprintf(Tide_buf, "%02d/%02d %02d:%02d %sft ", 
            levelTime.month(), levelTime.day(), levelTime.hour(), 
            levelTime.minute(), level_d);
    Serial.print(Tide_buf);
    Serial.print(x_plot);                               Serial.print(" ");
    Serial.print(y_baseLine + (nextLevel * y_factor));  Serial.print(" ");
    Serial.println();
    
    tft.fillCircle(x_plot * x_factor, y_baseLine + (nextLevel * y_factor), 1, ST77XX_BLACK);
    x_plot ++;

    bool Slope, nextSlope;
    if(level < nextLevel) Slope = 1;
    if(level > nextLevel) Slope = 0;

    DateTime lastTime = levelTime;
    float lastLevel = level;
    bool lastSlope  = Slope;
    
    while(rounds <= 2) {
        DateTime nextTime(lastTime + TimeSpan(60));
        nextLevel = tideLevel.currentTide(nextTime);
        if(lastLevel < nextLevel) nextSlope = 1;
        if(lastLevel > nextLevel) nextSlope = 0;
        
        char level_d[4];
        char Tide_buf[25];
        dtostrf(nextLevel, 4, 2, level_d);
        sprintf(Tide_buf, "%02d/%02d %02d:%02d %sft ", 
                lastTime.month(), lastTime.day(), lastTime.hour(), 
                lastTime.minute(), level_d);
        Serial.print(Tide_buf);
        Serial.print(x_plot);                               Serial.print(" ");
        Serial.print(y_baseLine + (nextLevel * y_factor));  Serial.print(" ");
        Serial.println();
        
        tft.fillCircle(x_plot * x_factor, y_baseLine + (nextLevel * y_factor), 1, ST77XX_BLACK);

        if(lastTime.minute() == 30) tft.drawFastVLine(x_plot * x_factor, 85, 125, ST77XX_BLACK);

        if(nextSlope != lastSlope && lastSlope == 0) {
            tft.setCursor(0, line_LowTide);
            tft.print(Tide_buf);
            tft.print("LOW");
            tft.fillCircle(x_plot * x_factor, y_baseLine + (nextLevel * y_factor), 3, ST77XX_BLACK);
            
            Serial.print(" LOW");
            
            LowTide = lastTime;
            rounds ++;
        }
        else if(nextSlope != lastSlope && lastSlope == 1) {
            tft.setCursor(0, line_HighTide);
            tft.print(Tide_buf);
            tft.print("HIGH");
            tft.fillCircle(x_plot * x_factor, y_baseLine + (nextLevel * y_factor), 3, ST77XX_BLACK);            
            
            Serial.print(" HIGH");
            
            LowTide = lastTime;
            rounds ++;
        }

        lastTime  = nextTime;
        lastLevel = nextLevel;
        lastSlope = nextSlope;
        x_plot ++;

        Serial.println();
    }
}

void getSunEvents() {
    DateTime now = rtc.now();
    int Sunrise  = IndianRiverInlet.sunrise(now.year(), now.month(), now.day(), DST);
    char sunrise_buf[5];
    IndianRiverInlet.min2str(sunrise_buf, Sunrise);    
    
    int Sunset   = IndianRiverInlet.sunset(now.year(), now.month(), now.day(), DST);
    char sunset_buf[5];
    IndianRiverInlet.min2str(sunset_buf, Sunset);
    
    tft.setCursor(0, line_SunEvents);
    tft.print("Sunrise ");      tft.print(sunrise_buf[0]);
    tft.print(sunrise_buf[1]);  tft.print(":");
    tft.print(sunrise_buf[2]);  tft.print(sunrise_buf[3]);

    tft.print("|");

    tft.print("Sunset ");      tft.print(sunset_buf[0]);
    tft.print(sunset_buf[1]);   tft.print(":");
    tft.print(sunset_buf[2]);   tft.print(sunset_buf[3]);

    // Serial.print("Sunrise ");     Serial.print(sunrise_buf[0]);
    // Serial.print(sunrise_buf[1]); Serial.print(":");
    // Serial.print(sunrise_buf[2]); Serial.print(sunrise_buf[3]);

    // Serial.print(" | Sunset ");   Serial.print(sunset_buf[0]);
    // Serial.print(sunset_buf[1]);  Serial.print(":");
    // Serial.print(sunset_buf[2]);  Serial.print(sunset_buf[3]);
    // Serial.println();
}
