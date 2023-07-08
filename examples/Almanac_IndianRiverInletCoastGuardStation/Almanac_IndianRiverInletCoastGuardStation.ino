#include "RTClib.h"
#include "TidelibIndianRiverInletCoastGuardStation.h"
#include "Dusk2Dawn.h"

RTC_DS3231 rtc;                                                       // instantiate DS3231 Real Time Clock (RTC)

TideCalc tideLevel;
DateTime LowTide, HighTide;

float LATITUDE   = 38.609;
float LONGITUDE  = -75.062;
float GMT_OFFSET = -5;
bool DST = true;
Dusk2Dawn IndianRiverInlet(LATITUDE, LONGITUDE, GMT_OFFSET);

const long clockInterval = 60000;                                     // 1-min - clock update interval (ms)
const long tideInterval  = 3600000;                                   // 1-hour - tides update interval (ms)
const long sunInterval   = 3600000;                                   // 1-hour interval update sets (ms)

void setup() {
    Wire.begin();
    Serial.begin(57600);
    rtc.begin();
    if(rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    delay(50);

    Serial.println("*******************************************************");
    displayDateTime();
    displayTideEvents();
    getSunEvents();
}

void loop() {
    unsigned long clockTicks, tideTicks, sunTicks;
    unsigned long currentMillis = millis();
    if (currentMillis - clockTicks >= clockInterval) {                // update clock
        clockTicks = currentMillis;
        displayDateTime();
    }
    if (currentMillis - tideTicks >= tideInterval) {                  // update tide displays
        tideTicks = currentMillis;
        displayTideEvents();
    }
    if (currentMillis - sunTicks >= sunInterval) {                    // update sun displays
        sunTicks = currentMillis;
        getSunEvents();
    }
}
 
void displayDateTime() {
    DateTime now = rtc.now();                                         // get current time from DS3231 RTC
    char now_datetime_buf[] = "DDD MM/DD/YY hh:mm:ss"; 
    Serial.println(now.toString(now_datetime_buf));
}

void displayTideEvents() {                                                           
    DateTime levelTime = rtc.now();                                   // get current time from DS3231 RTC
    float level = tideLevel.currentTide(levelTime);

    DateTime nextTime(levelTime + TimeSpan(60));                      // add 60 seconds
    float nextLevel = tideLevel.currentTide(nextTime);

    char level_d[4];
    char Tide_buf[25];
    dtostrf(level, 4, 2, level_d);
    sprintf(Tide_buf, "%02d/%02d %02d:%02d %sft", 
            levelTime.month(), levelTime.day(), levelTime.hour(), levelTime.minute(), level_d);
    Serial.print(Tide_buf);
    Serial.println();

    bool Slope, nextSlope;
    if(level < nextLevel) Slope = 1;
    if(level > nextLevel) Slope = 0;

    DateTime lastTime = levelTime;
    float lastLevel = level;
    bool lastSlope  = Slope;
    int rounds = 1;

    while(rounds <= 3) {
        DateTime nextTime(lastTime + TimeSpan(60));
        nextLevel = tideLevel.currentTide(nextTime);
        if(lastLevel < nextLevel) nextSlope = 1;
        if(lastLevel > nextLevel) nextSlope = 0;
        
        char level_d[4];
        char Tide_buf[25];
        dtostrf(nextLevel, 4, 2, level_d);
        sprintf(Tide_buf, "%02d/%02d %02d:%02d %sft", 
        lastTime.month(), lastTime.day(), lastTime.hour(), lastTime.minute(), level_d);
        Serial.print(Tide_buf);
        
        if(nextSlope != lastSlope && lastSlope == 0) {
            Serial.print(" - LOW");
            LowTide = lastTime;
            rounds ++;
        }
        else if(nextSlope != lastSlope && lastSlope == 1) {
            Serial.print(" - HIGH");
            rounds ++;
        }

        lastTime  = nextTime;
        lastLevel = nextLevel;
        lastSlope = nextSlope;
        Serial.println();
    }
}

void getSunEvents() {
    DateTime now = rtc.now();
    int Sunrise  = IndianRiverInlet.sunrise(now.year(), now.month(), now.day(), DST);
    int Sunset   = IndianRiverInlet.sunset(now.year(), now.month(), now.day(), DST);

    char sunrise_buf[5];
    IndianRiverInlet.min2str(sunrise_buf, Sunrise);
    Serial.print("Sunrise ");     Serial.print(sunrise_buf[0]);
    Serial.print(sunrise_buf[1]); Serial.print(":");
    Serial.print(sunrise_buf[2]); Serial.print(sunrise_buf[3]);

    char sunset_buf[5];
    IndianRiverInlet.min2str(sunset_buf, Sunset);
    Serial.print(" | Sunset ");   Serial.print(sunset_buf[0]);
    Serial.print(sunset_buf[1]);  Serial.print(":");
    Serial.print(sunset_buf[2]);  Serial.print(sunset_buf[3]);
    Serial.println();
}
