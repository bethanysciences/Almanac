#include <RTClib.h>
#include <TimeLib.h>          // www.nongnu.org/avr-libc/user-manual/group__avr__time.html
#include <locale.h>
#include <util/usa_dst.h>     // nongnu.org/avr-libc/user-manual/usa__dst_8h_source.html

#define DEBUG 1               // 1 do serial prints for debug

RTC_DS3231 rtc;
unsigned long LastUnSyncTime = 0;
time_t time_provider()
{
    return rtc.now().unixtime();  
}
#define TIME_ZONE     -5                                  // Diff from GMT
#define LATITUDE      27.97                               // Clearwater Beach CWFB1 Station
#define LONGITUDE     -82.83
char LSD[3][4]        = {"LST", "LDT"};
tmElements_t tm;
char TimeSync[4][15]  = {"not set", "not sync", "sync"};

#include <TM1637.h>
int CLK  = 12;                                            // common clock line
TM1637 riseDisplay(CLK, 8);                               // sunrise display (yellow 5463BY)
TM1637 setDisplay(CLK, 9);                                // sunset display (red 5463BS-7.3)
TM1637 highDisplay(CLK, 11);                              // high tide display (white 5463BW-7.3)
TM1637 lowDisplay(CLK, 3);                                // low tide display (blue 5463BB)

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH    128                               // OLED display width, in pixels
#define SCREEN_HEIGHT   32                                // OLED display height, in pixels
#define OLED_RESET      -1                                // Reset pin
#define SCREEN_ADDRESS  0x3C                              // 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int ledPin =      LED_BUILTIN;                      // heartbeat LED pin
int ledState =          LOW;

#include "TidelibClearwaterBeachGulfOfMexicoFlorida.h"
TideCalc myTideCalc;

void setup() {
    #ifdef DEBUG
        Serial.begin(57600);
        Serial.println("START");
    #endif
    set_dst(usa_dst);
    set_zone(TIME_ZONE * ONE_HOUR);
    set_position(LATITUDE * ONE_DEGREE, LONGITUDE * ONE_DEGREE);
    if (! rtc.begin()) while (1);
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // rtc.adjust(DateTime(2023, 3, 12, 6, 59, 50));    // lst > ldt transistion 2nd SUN MAR
    // rtc.adjust(DateTime(2023, 11, 3, 6, 59, 50));    // ldt > lst transistion 1st SUN NOV
    
    DateTime now = rtc.now();
    setSyncProvider(time_provider);
    setSyncInterval(5);
    long SetSystemTimeUTC = (now.unixtime() - UNIX_OFFSET);
    set_system_time(SetSystemTimeUTC);

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
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display.setCursor(0, 0);
    display.print("Start");
    display.display();

    delay(2000);

    // displayTime();
    displayAlmanac();
}   

void loop() {
    tickUpdate();
}

void tickUpdate() {
    static unsigned long lastTickTime;
    static int syncIntervalCount = 0;
    if (micros() - lastTickTime >= 1000000) {
        lastTickTime += 1000000;
        system_tick();
        if (ledState == LOW) ledState = HIGH;       // heartbeat
        else ledState = LOW;
        digitalWrite(ledPin, ledState);
    }
    // displayTime();
    displayAlmanac();
}

void displayTime() {
    char timeLOCbuf[24];
    char dateLOCbuf[24];
    time_t locTime = time(NULL);
    struct tm * nowtimeLOC = localtime(&locTime);
    strftime(timeLOCbuf, 24, "%H:%M", localtime(&locTime));
    strftime(dateLOCbuf, 24, "%m/%d/%Y", localtime(&locTime));

    display.setCursor(0, 0);
    display.print(timeLOCbuf);
    display.println(nowtimeLOC->tm_isdst ? " LDT" : " LST");
    display.print(dateLOCbuf);
    display.display();
}

void displayAlmanac() {
    char timeLOCbuf[10];
    char dateLOCbuf[12];
    time_t locTime = time(NULL);
    struct tm * nowtimeLOC = localtime(&locTime);
    strftime(timeLOCbuf, 10, "%H:%M:%S", localtime(&locTime));
    strftime(dateLOCbuf, 12, "%m/%d/%Y", localtime(&locTime));
    
    display.setCursor(0, 0);
    display.print(timeLOCbuf);
    display.println(nowtimeLOC->tm_isdst ? "D" : "S");
    display.print(dateLOCbuf);
    display.display();

    #ifdef DEBUG
        Serial.print(dateLOCbuf);
        Serial.print(" ");
        Serial.print(timeLOCbuf);
        Serial.print(nowtimeLOC->tm_isdst ? " LDT " : " LST ");
        Serial.print(TimeSync[timeStatus()]);
    #endif

    time_t timeNow;
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
    float INTERVAL        = 6 * 60L;        // check every 6 mins (as NOAA does)
    float results;
    timeNow = time(NULL);
    
    DateTime now = rtc.now();
    DateTime adjnow(now.unixtime() - (localtime(&timeNow)->tm_isdst));
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
                DateTime adjHigh(futureHigh.unixtime() + (localtime(&timeNow)->tm_isdst));
                highTide = adjHigh.toString(bufHigh);
                highDisplay.display(highTide);

                DateTime adjLow(futureLow.unixtime() + (localtime(&timeNow)->tm_isdst));
                lowTide = adjLow.toString(bufLow);
                lowDisplay.display(lowTide);

                #ifdef DEBUG
                    Serial.print(" | high ");  Serial.print(highTide);
                    Serial.print(" low ");  Serial.print(lowTide);
                #endif
            }
            else {
                DateTime adjLow(futureLow.unixtime() + (localtime(&timeNow)->tm_isdst));
                lowTide = adjLow.toString(bufLow);
                lowDisplay.display(lowTide);

                DateTime adjHigh(futureHigh.unixtime() + (localtime(&timeNow)->tm_isdst));
                highTide = adjHigh.toString(bufHigh);
                highDisplay.display(highTide);
                
                #ifdef DEBUG
                    Serial.print(" | high ");  Serial.print(highTide);
                    Serial.print(" low ");  Serial.print(lowTide);
                #endif
            }
            results = myTideCalc.currentTide(adjnow);
            gate = 1;
            bing = 0;
            futureHighGate = 0;
            futureLowGate  = 0;
        }
        pastResult = results;
    }
    char sunRisebuff[6];
    time_t SuntimeNow = time(NULL);
    time_t sunrise = sun_rise(&SuntimeNow);
    strftime(sunRisebuff, 6, "%H%M ", localtime(&sunrise));

    char sunSetbuff[6];
    time_t sunset = sun_set(&SuntimeNow);
    strftime(sunSetbuff, 6, "%H%M", localtime(&sunset));

    #ifdef DEBUG
        Serial.print(" | rise ");   Serial.print(sunRisebuff);
        Serial.print(" set ");      Serial.println(sunSetbuff);
    #endif
}