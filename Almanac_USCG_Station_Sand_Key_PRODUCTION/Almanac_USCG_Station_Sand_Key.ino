#include <RTClib.h>

#include <TM1637.h>
int CLK  = 12;                                            // common clock line
TM1637 riseDisplay(CLK, 8);                               // sunrise display (yellow 5463BY)
TM1637 setDisplay(CLK, 9);                                // sunset display (red 5463BS-7.3)
TM1637 highDisplay(CLK, 11);                              // high tide display (white 5463BW-7.3)
TM1637 lowDisplay(CLK, 3);                                // low tide display (blue 5463BB)

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128                                  // OLED display width, in pixels
#define SCREEN_HEIGHT 32                                  // OLED display height, in pixels
#define OLED_RESET     -1                                 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C                               //< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int ledPin = LED_BUILTIN;                           // the number of the LED pin
int ledState = LOW;                                       // ledState sets LED
unsigned long previousMillis = 0;                         // stores last time LED updated
const long interval = 1000;                               // heatbeat interval (milliseconds)

RTC_DS3231 rtc;
char LSD[3][4] = {"LST", "LDT"};

#include "TidelibClearwaterBeachGulfOfMexicoFlorida.h"
TideCalc myTideCalc;

#include <Dusk2Dawn.h>
Dusk2Dawn clearwater(27.97, -82.83, -5);                   // Station CWBF1 - 8726724 - Clearwater Beach, FL

void setup() {
    highDisplay.init();
    lowDisplay.init();
    riseDisplay.init();
    setDisplay.init();
    highDisplay.setBrightness(7);
    lowDisplay.setBrightness(7);
    riseDisplay.setBrightness(7);
    setDisplay.setBrightness(7);
    
    pinMode(ledPin, OUTPUT);                               // set up heartbeat

    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.setRotation(2);
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Start");
    display.display();

    if (! rtc.begin()) while (1);
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    delay(2000);
}   

void loop() {
    getTime();
    getSunEvents();
    getTideEvents();

    unsigned long currentMillis = millis();               // heartbeat timer
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;                   // save the last heartbeat
        if (ledState == LOW) ledState = HIGH;
        else ledState = LOW;
        digitalWrite(ledPin, ledState);
    }
    delay(3000);
}

void getTime() {
    DateTime now = rtc.now();                             // get current time from RTC
    // DateTime adjnow(now.unixtime() - (isLDT(now) * 3600));
    display.clearDisplay();
    display.setCursor(0, 0);

    char curr_date_buf[] = "MM/DD/YYYY";
    // display.println(adjnow.toString(curr_date_buf));
    display.println(now.toString(curr_date_buf));

    char curr_time_buf[] = "hh:mm ";
    // display.print(adjnow.toString(curr_time_buf));
    display.print(now.toString(curr_time_buf));
    display.print(LSD[isLDT(now)]);
    display.display();
}

bool isLDT(const DateTime& curLST) {
    if (curLST.month() >= 3) return true;
    else return false;
}

void getTideEvents() {
    DateTime futureHigh;
    DateTime futureLow;
    DateTime future;
    String highTide;
    String lowTide;
    char bufHigh[] = "hhmm";
    char bufLow[]  = "hhmm";

    int   slope;
    int   i               = 0;
    int   zag             = 0;
    bool  gate            = 1;
    float tidalDifference = 0;
    bool  bing            = 1;
    bool  futureLowGate   = 0;
    bool  futureHighGate  = 0;
    float INTERVAL        = 3 * 60L;
    float results;
    
    DateTime now = rtc.now();
    DateTime adjnow(now.unixtime() - (isLDT(now) * 3600));
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
                highTide = adjHigh.toString(bufHigh);
                highDisplay.display(highTide);

                DateTime adjLow(futureLow.unixtime() + (isLDT(futureLow) * 3600));
                lowTide = adjLow.toString(bufLow);
                lowDisplay.display(lowTide);
            }
            else {
                DateTime adjLow(futureLow.unixtime() + (isLDT(futureLow) * 3600));
                lowTide = adjLow.toString(bufLow);
                lowDisplay.display(lowTide);

                DateTime adjHigh(futureHigh.unixtime() + (isLDT(futureHigh) * 3600));
                highTide = adjHigh.toString(bufHigh);
                highDisplay.display(highTide);
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
    int Sunrise = clearwater.sunrise(now.year(), now.month(), now.day(), isLDT(now));
    char risebuff[] = "0000";
    Dusk2Dawn::min2str(risebuff, Sunrise);
    String riseStr(risebuff);
    riseDisplay.display(riseStr);

    int Sunset = clearwater.sunset(now.year(), now.month(), now.day(), isLDT(now));
    char setbuff[] = "0000";
    Dusk2Dawn::min2str(setbuff, Sunset);   
    String setStr(setbuff);
    setDisplay.display(setStr); 
}
