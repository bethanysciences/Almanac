#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128                  // OLED display width, in pixels
#define SCREEN_HEIGHT 32                  // OLED display height, in pixels
#define OLED_RESET     -1                 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C               //< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include "RTClib.h"
RTC_DS3231 rtc;                                     // use to instantiate DS3231 RTC module
// RTC_DS1307 rtc;                                     // use to instantiate DS1307 RTC module
// RTC_Millis rtc;                                  // use w/o clock module

#define USE_USLDT           true                    // location uses US daylight savings
char LOC[3][4]              = {"LST","LDT"};        // Local Standard Time (LST)
                                                    // Local Daylight Time (LDT)
#define GMT_OFFSET          -5                      // LST hours GMT offset

// ------------  SUN EVENTS  ---------------------- //
#include <Dusk2Dawn.h>
Dusk2Dawn clearwater(27.978, -82.832, -5);          // Station CWBF1 - 8726724 - Clearwater Beach, FL

// ------------  TIDE CALCULATIONS  --------------- //
// #include "TidelibDelawareCityDelawareRiverDelaware.h"
#include "TidelibClearwaterBeachGulfOfMexicoFlorida.h"
TideCalc myTideCalc;

void setup() {
    Wire.begin();

    asm(".global _printf_float");                               // printf renders floats

    Serial.begin(57600);
    while (!Serial);                                            // wait for serial port to open

    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {  // SSD1306_SWITCHCAPVCC = generate voltage
        Serial.println("SSD1306 allocation failed");
        for(;;);                                                  // Don't proceed, loop forever
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    display.println("Start");
    display.display();

    if (! rtc.begin()) {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        while (1) delay(10);
    }

    if (rtc.lostPower()) {
        Serial.println("RTC lost power, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    statusScreen();

    getTideEvents();
    getSunEvents();
}

void loop() {
  delay(10000);
  displayTime();
}

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

void displayTime() {
    DateTime now = rtc.now();                                   // get current time from RTC

    char curr_date_buf[] = "MM/DD/YYYY";
    Serial.print(now.toString(curr_date_buf));

    char curr_time_buf[] = "hh:mm:ss";
    Serial.print(now.toString(curr_time_buf));

    Serial.println(LOC[isLDT(now)]);                // render LST or LDT

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(now.toString(curr_time_buf));
    display.println(now.toString(curr_date_buf));
    display.display();
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

    DateTime now = rtc.now();                       // current time from RTC
    DateTime adjnow(now.unixtime() 
             - (isLDT(now) * 3600));                // convert to LST during LDT

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

                char high_buf[] = "Next High tide MM/DD/YY hh:mm ";
                Serial.print(adjHigh.toString(high_buf));
                Serial.print(LOC[isLDT(adjHigh)]);            // render LST or LDT
                Serial.print(" level ");
                Serial.print(resultsHigh, 2);
                Serial.println(" feet");

                DateTime adjLow(futureLow.unixtime() + 
                         (isLDT(futureLow) * 3600));

                char low_buf[] = "Next Low tide  MM/DD/YY hh:mm ";
                Serial.print(adjLow.toString(low_buf));
                Serial.print(LOC[isLDT(adjLow)]);            // render LST or LDT
                Serial.print(" level ");
                Serial.print(resultsLow, 2);
                Serial.println(" feet");

            }
            else {
                DateTime adjLow(futureLow.unixtime() + 
                         (isLDT(futureLow) * 3600));

                char low_buf[] = "Next Low tide  MM/DD/YY hh:mm ";
                Serial.print(adjLow.toString(low_buf));
                Serial.print(LOC[isLDT(adjLow)]);            // render LST or LDT
                Serial.print(" level ");
                Serial.print(resultsLow, 2);
                Serial.println(" feet");

                DateTime adjHigh(futureHigh.unixtime() + 
                         (isLDT(futureHigh) * 3600));

                char high_buf[] = "Next High tide MM/DD/YY hh:mm ";
                Serial.print(adjHigh.toString(high_buf));
                Serial.print(LOC[isLDT(adjHigh)]);            // render LST or LDT
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

void getSunEvents() {
    DateTime now = rtc.now();                            // current time from RTC
    int Sunrise = clearwater.sunrise(now.year(), now.month(), now.day(), isLDT(now));  // returns mins past midnight
    char risebuff[5];
    Dusk2Dawn::min2str(risebuff, Sunrise);
    Serial.print("Sunrise "); Serial.println(risebuff);

    int Sunset = clearwater.sunset(now.year(), now.month(), now.day(), isLDT(now));  // returns mins past midnight
    char setbuff[5];
    Dusk2Dawn::min2str(setbuff, Sunset);
    Serial.print("Sunset "); Serial.println(setbuff);
}

void statusScreen() {
    Serial.println("-----------------------------------------------------------------------");

    DateTime now = rtc.now();                                   // get current time from RTC

    Serial.print("current time\t\t");
    char curr_date_buf[] = "DDD MM/DD/YY ";
    Serial.print(now.toString(curr_date_buf));
    
    char curr_time_buf[] = "hh:mm:ss ";
    Serial.print(now.toString(curr_time_buf));
    Serial.println(LOC[isLDT(now)]);                            // render LST or LDT

    // --- Get and Display DS3231SN RTC Paramters ---
    Serial.println("DS3231SN Paramters");
    Serial.print("lostPower()\t");
    Serial.print(rtc.lostPower());
    Serial.print("\treadSqwPinMode()\t");
    Serial.println(rtc.readSqwPinMode());

    Serial.print("isEnabled32K()\t");
    Serial.print(rtc.isEnabled32K());
    Serial.print("\tchip temperature\t");
    Serial.print(rtc.getTemperature());
    Serial.println(" C");

    Serial.println("-----------------------------------------------------------------------");
}


