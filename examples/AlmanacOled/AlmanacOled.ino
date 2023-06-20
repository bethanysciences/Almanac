// ---------------  SERIAL PRINT DEBUG SWITCH ------------- //
#define DEBUG false                                          // true setup and use serial print


// ---------------  WiFi Service  ------------------------- //
#include <SPI.h>
#include<WiFiNINA.h>
#include<WiFiUdp.h>

char ssid[]                     = "Del Boca Vista";
char pass[]                     = "francine";
uint8_t status                  = WL_IDLE_STATUS;


// ---------------  SAMD Real Time Clock  ----------------- //
#include<rtcSAMD.h>
#include <time.h>

rtcSAMD rtc;
uint32_t epoch;
#define EPOCH_TIME_OFF          946684800                   // 1/1/2000 00:00:00 epoch time
#define EPOCH_TIME_YEAR_OFF     100                         // years since 1900
#define LST_OFFSET              -5                          // LST hours GMT offset
#define USE_USLDT               true                        // US daylight savings?
char ampm[2][3]                 = {"am", "pm"};             // AM/PM
char LOC[3][4]                  = {"LST",                   // Local Standard Time (LST)
                                   "LDT"};                  // Local Daylight Time (LDT)


// ---------------  SUN EVENTS  --------------------------- //
#include <sunevents.h>                                      // version=0.1.0

float LATITUDE                  = 38.61;                    // Bethany Beach, DE, USA
float LONGITUDE                 = -75.07;
#define LDT                     true
sunevents bethany(LATITUDE, LONGITUDE, LST_OFFSET);


// ---------------  TIDES  -------------------------------- //
#include <tides.h>
TideCalc myTideCalc;


// ---------------  OLED DISPLAY  ------------------------- //
#include <U8g2lib.h>
U8G2_SSD1327_MIDAS_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#define LINE_HEIGHT             14
#define LOCATION_LINE           14
#define TIME_LINE               30
#define RISE_LINE               54
#define SET_LINE                76
#define LOW_LINE                98
#define HIGH_LINE               121


void setup() {
    asm(".global _printf_float");                           // printf renders floats

#if DEBUG
    Serial.begin(115200);
    while (!Serial);                                        // wait for serial port to open
    Serial.println("\nsetup -------------------------------------------------------------");
#endif

    oledSetup();

    delay(1000);

    rtc.begin();
    rtc.enableAlarm(rtc.MATCH_SS);                          // fire every minute
    rtc.attachInterrupt(chronoFire);

    wifiNTPSetup();
    wifiStrength();
    displayTime();
    Sun();
    Tides();
}
void chronoFire() {                                         // update time each minute
    wifiStrength();
    displayTime();
    Sun();
    Tides();
}
void oledSetup() {

#if DEBUG
    Serial.println("oledSetup -- ");
#endif

    u8g2.setI2CAddress(0x78);
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);

    u8g2.drawFrame(0, 34, 127, 93);
    u8g2.drawHLine(0,   57,  127);
    u8g2.drawHLine(0,   80,  127);
    u8g2.drawHLine(0,  105,  127);
    u8g2.drawVLine(48,  34,  93);

    u8g2.setFont(u8g2_font_crox1h_tf);
    char location_0[] = "Bethany Beach, DE";
    u8g2.setCursor((u8g2.getDisplayWidth() - u8g2.getStrWidth(location_0)) / 2, LOCATION_LINE);
    u8g2.print(location_0);

    u8g2.setFont(u8g2_font_helvR14_te);
    u8g2.setCursor(4, RISE_LINE);   u8g2.print("Rise");
    u8g2.setCursor(4, SET_LINE);    u8g2.print("Set");
    u8g2.setCursor(4, HIGH_LINE);   u8g2.print("High");
    u8g2.setCursor(4, LOW_LINE);    u8g2.print("Low");

    u8g2.sendBuffer();
}
void wifiNTPSetup() {

#if DEBUG
    Serial.println("wifiNTPSetup ----- ");
#endif

    while (status != WL_CONNECTED) {

#if DEBUG
        Serial.print("conn ");  Serial.print(ssid);
#endif

        status = WiFi.begin(ssid, pass);
        delay(10000);
  }

#if DEBUG
        IPAddress ip = WiFi.localIP();
        Serial.print(" / addr ");
        Serial.print(ip);
        long rssi = WiFi.RSSI();
        Serial.print(" / rssi ");
        Serial.print(rssi);
        Serial.println(" dBm");

        Serial.print("NTP time - ");
#endif

        int numberOfTries = 0, maxTries = 6;

    do {
        epoch = WiFi.getTime();
        numberOfTries++;
        if (epoch == 0) delay(2000);
    }

    while ((epoch == 0) && (numberOfTries < maxTries));
    if (numberOfTries >= maxTries) {

#if DEBUG
        Serial.print("NTP unreachable ");
#endif

    }
    else rtc.setEpoch(epoch + (LST_OFFSET * 3600) + (isLDT(epoch) * 3600));

#if DEBUG
    Serial.print("Epoch: ");    Serial.println(epoch);
#endif

}
void wifiStrength() {
    u8g2.setDrawColor(1);
    if (WiFi.RSSI() > -55) {
        u8g2.drawBox(114, 12, 8, 2);
        u8g2.drawBox(117, 10, 6, 4);
        u8g2.drawBox(120, 8,  4, 6);
        u8g2.drawBox(123, 6,  2, 8);
        u8g2.drawBox(126, 2,  0, 10);
    }
    else if (WiFi.RSSI() < -55 & WiFi.RSSI() > -65) {
        u8g2.drawBox(117, 12, 8, 2);
        u8g2.drawBox(120, 10, 6, 4);
        u8g2.drawBox(123, 8,  4, 6);
        u8g2.drawBox(126, 6,  2, 8);
    }
    else if (WiFi.RSSI() < -65 & WiFi.RSSI() > -70) {
        u8g2.drawBox(120, 12, 8, 2);
        u8g2.drawBox(123, 10, 6, 4);
        u8g2.drawBox(126, 8,  4, 6);
    }
    else if (WiFi.RSSI() < -70 & WiFi.RSSI() > -78) {
        u8g2.drawBox(123, 12, 8, 2);
        u8g2.drawBox(126, 10, 6, 4);
    }
    else if (WiFi.RSSI() < -78 & WiFi.RSSI() > -82) {
        u8g2.drawBox(126, 12, 8, 2);
    }
    u8g2.sendBuffer();

#if DEBUG
    Serial.print("WiFi ----- "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
#endif

}
bool isLDT(uint32_t test_epoch) {
    time_t t = test_epoch;
    struct tm* tmp = gmtime(&t);
      int year = tmp->tm_year - EPOCH_TIME_YEAR_OFF + 2000;
      int month = tmp->tm_mon + 1;
      int day = tmp->tm_mday;
      int hour = tmp->tm_hour;
      int minute = tmp->tm_min;
      int second = tmp->tm_sec;

  if (!USE_USLDT) return false;
  int y = year - 2000;
  int x = (y + y / 4 + 2) % 7;
  if (month == 3 && day == (14 - x) && hour >= 2) return true;
  if (month == 3 && day > (14 - x) || month > 3) return true;
  if (month == 11 && day == (7 - x) && hour >= 2) return false;
  if (month == 11 && day > (7 - x) || month > 11 || month < 3) return false;
}
bool is_PM(int hr) {
    if (hr >= 12) return true;
    else return false;
}
void displayTime() {
    int hour = rtc.getHours();
    bool pm = false;
    if (hour >= 12) pm = true;
    if (hour > 12) hour -= 12;
    if (hour == 0) hour = 12;
    
    char time_buf[20];
    sprintf(time_buf, "%02d/%02d %d:%02d%s",
                       rtc.getMonth(), rtc.getDay(), 
                       hour, rtc.getMinutes(), ampm[pm]);
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, TIME_LINE - 12, 127, 12);

    u8g2.setFont(u8g2_font_crox1h_tf);
    u8g2.setDrawColor(1);
    u8g2.setCursor((u8g2.getDisplayWidth() - u8g2.getStrWidth(time_buf)) / 2, TIME_LINE);
    u8g2.print(time_buf);

    u8g2.sendBuffer();

#if DEBUG
  Serial.print("\nTime ----- ");
  Serial.println(time_buf);
#endif

}
void displayTides(char dataLabel[10], uint32_t epoch, float height) {
    time_t t = epoch;
    struct tm* tmp = gmtime(&t);
    int year = tmp->tm_year - EPOCH_TIME_YEAR_OFF + 2000;
    int month = tmp->tm_mon + 1;
    int day = tmp->tm_mday;
    int hour = tmp->tm_hour;
    int minute = tmp->tm_min;
    int second = tmp->tm_sec;

    bool pm = false;
    if (hour >= 12) pm = true;
    if (hour > 12) hour -= 12;
    if (hour == 0) hour = 12;
    
    char buf[40] = "\0";
    sprintf(buf, "%d:%02d%s", hour, minute, ampm[pm]);

    if (dataLabel == "Low ") {
        u8g2.setDrawColor(0);
        u8g2.drawBox(52, LOW_LINE + 4 - LINE_HEIGHT, 74, LINE_HEIGHT);
        u8g2.setFont(u8g2_font_helvR14_te);
        u8g2.setDrawColor(1);
        u8g2.setCursor(52, LOW_LINE);
        u8g2.print(buf);
        u8g2.sendBuffer();
    }
    if (dataLabel == "High") {
        u8g2.setDrawColor(0);
        u8g2.drawBox(52, HIGH_LINE - LINE_HEIGHT, 74, LINE_HEIGHT);
        u8g2.setFont(u8g2_font_helvR14_te);
        u8g2.setDrawColor(1);
        u8g2.setCursor(52, HIGH_LINE);
        u8g2.print(buf);
        u8g2.drawVLine(48, 77, 127);
        u8g2.sendBuffer();
    }

#if DEBUG
    char buf_debug[80];
    sprintf(buf_debug, "%s ----- %02d/%02d %d:%02d%s height %2.4f ft.",
            dataLabel, month, day, hour, minute, ampm[pm], height);
    Serial.println(buf_debug);
#endif

}
void Sun() {
    char rise_buf[20] = "\0";
    char set_buf[20]  = "\0";

    int bethanyRise = bethany.sunrise(rtc.getYear(), rtc.getMonth(), rtc.getDay(), LDT);
    int bethanySet = bethany.sunset(rtc.getYear(), rtc.getMonth(), rtc.getDay(), LDT);

    int riseHour = bethany.mintutes2Hour(bethanyRise);
    if (riseHour > 12) riseHour -= 12;
    if (riseHour == 0) riseHour = 12;
    int riseMinute = bethany.mintutes2Minute(bethanyRise);

    int setHour = bethany.mintutes2Hour(bethanySet);
    if (setHour > 12) setHour -= 12;
    if (setHour == 0) setHour = 12;
    int setMinute = bethany.mintutes2Minute(bethanySet);

    sprintf(rise_buf, "%d:%2dam", riseHour, riseMinute);
    sprintf(set_buf,  "%d:%2dpm", setHour,  setMinute);

    u8g2.setDrawColor(0);
    u8g2.drawBox(52, RISE_LINE - LINE_HEIGHT, 74, LINE_HEIGHT);
    u8g2.drawBox(52, SET_LINE -  LINE_HEIGHT, 74, LINE_HEIGHT);

    u8g2.setFont(u8g2_font_helvR14_te);
    u8g2.setDrawColor(1);
    u8g2.setCursor(52, RISE_LINE);
    u8g2.print(rise_buf);
    u8g2.setCursor(52, SET_LINE);
    u8g2.print(set_buf);
    u8g2.sendBuffer();

#if DEBUG
    Serial.print("Sun ------ ");
    Serial.print(rise_buf);
    Serial.print("  ");
    Serial.println(set_buf);
#endif

}

void Tides() {
    float INTERVAL          = 1 * 5 * 60L;              // tide calc interval
    uint32_t adjHigh, adjLow;
    uint32_t futureHigh, futureLow, future;
    uint8_t slope;
    uint8_t i               = 0;
    uint8_t zag             = 0;
    bool    gate            = 1;
    float   results;
    float   tidalDifference = 0;
    bool    bing            = 1;
    bool    futureLowGate   = 0;
    bool    futureHighGate  = 0;
    uint32_t epoch          = rtc.getEpoch();
    uint32_t adjnow         = epoch - (isLDT(epoch) * 3600);
    float pastResult        = myTideCalc.currentHeight(adjnow);

    while(bing){
        i++;
        future = adjnow + (i * INTERVAL);
        results = myTideCalc.currentHeight(future);
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
            float resultsHigh = myTideCalc.currentHeight(futureHigh);
            float resultsLow  = myTideCalc.currentHeight(futureLow);
        
            if((futureHigh - futureLow) < 0) hiLow = 1;
            if((futureHigh - futureLow) > 0) hiLow = 0;
            
            if (hiLow) {
                adjHigh = futureHigh + (isLDT(futureHigh) * 3600);
                displayTides("High", adjHigh, resultsHigh);
                // highDisplay.dispNumber((adjHigh.hour() * 100) + adjHigh.minute());

                adjLow = futureLow + (isLDT(futureLow) * 3600);
                displayTides("Low ", adjLow,  resultsLow);
                // lowDisplay.dispNumber((adjLow.hour() * 100) + adjLow.minute());
                
            }
            else {
                adjLow = futureLow + (isLDT(futureLow) * 3600);
                displayTides("Low ", adjLow,  resultsLow);              
                // lowDisplay.dispNumber((adjLow.hour() * 100) + adjLow.minute());

                adjHigh = futureHigh + (isLDT(futureHigh) * 3600);
                displayTides("High", adjHigh, resultsHigh);                
                // highDisplay.dispNumber((adjHigh.hour() * 100) + adjHigh.minute());

            }
            results = myTideCalc.currentHeight(adjnow);
            gate = 1;
            bing = 0;
            futureHighGate = 0;
            futureLowGate  = 0;
        }
        pastResult = results;
    }
}

void loop() { }
