#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "TidelibDelawareCityDelawareRiverDelaware.h"

RTC_DS3231 rtc;

#define TFT_CS        10
#define TFT_RST        9
#define TFT_DC         8

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

const int8_t iChartCol=32;
int8_t iChartData[32];
float fChartData[32];
int8_t iChartDataDec[32];
int8_t iMinMax[4];

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

    // getChartData();
    // getMinMaxData();

    Serial.println("Tide levels 5-min increments 24-hours");
    for( int8_t x=1; x <= iChartCol; x++ ) {
        Serial.print("Tide height: ");
        //Serial.print(iChartData[x-1],3);
        Serial.print(ChartData[x-1],3);
        //Serial.print(".");Serial.print(iChartDataDec[x-1]);
        Serial.print(" ft.");
        Serial.println();
    }
    Serial.println();
}

void loop() {
    static int8_t iLastHour = 24;
    if( rtc.now().hour() == 0 && iLastHour != 0){ // new day
        getChartData();
        getMinMaxData();
        iLastHour=0;
    }
    else iLastHour = rtc.now().hour();
}

/*
void timeLine(uint32_t color, int wait) {
    int8_t iCol, iTimeCol;
    int8_t iHour, iMinutes;
    static int8_t iCurCol=0;

    DateTime curTime;
    curTime = rtc.now();
    iHour = curTime.hour();
    iMinutes = curTime.minute();
    iTimeCol=abs((iHour*60+iMinutes)/45);  
    for(uint16_t i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
        // check for which column
        iCol = i/MATRIX_WIDTH;
        //Serial.print("column:");Serial.println(iCol);
        if( iCol == iTimeCol ) { // true == even
            if( iCol != iCurCol ) // clear previous line
                for(int8_t j=0; j<MATRIX_WIDTH; j++)
                strip.setPixelColor(iCurCol*MATRIX_WIDTH+j, strip.Color(0, 0, 0)); // Clear pixel's
            for(int8_t j=0; j<MATRIX_WIDTH; j++) {   // test if pixel color is already set, skip if isSet
                if( strip.getPixelColor(iCol*MATRIX_WIDTH+j) == 0 ) 
                strip.setPixelColor(iCol*MATRIX_WIDTH+j, color);      //  Set pixel's color (in RAM)
            }
            iCurCol=iCol;
        }
        strip.show();                          //  Update strip to match
        delay(wait);                           //  Pause for a moment
    }
}

void minMaxPeaks(uint32_t color, int wait) {
    int8_t iCol;
    uint32_t iColor, iBrightColor;
    iBrightColor=strip.Color(255,127,127);
    for(uint16_t i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
        // check for which column
        iCol = i/MATRIX_WIDTH;
        //Serial.print("column:");Serial.println(iCol);
        if( iCol==iMinMax[0] || iCol==iMinMax[1] || iCol==iMinMax[2] || iCol==iMinMax[3] ){
            color = iBrightColor;
            if( (iCol % 2) == 0 ) // true == even
                strip.setPixelColor(7+(iCol*MATRIX_WIDTH)-iChartData[iCol], color);         //  Set pixel's color (in RAM)
            else // odd
                strip.setPixelColor((iCol*MATRIX_WIDTH)+iChartData[iCol], color);         //  Set pixel's color (in RAM)
        }
        else color = iColor; 
        strip.show();                          //  Update strip to match
        delay(wait);                           //  Pause for a moment
    }
}
*/

void getChartData() {
    TideCalc myTideCalc; // Create TideCalc object called myTideCalc
    float result;
    DateTime today, startTime;
    today = rtc.now();
    Serial.print(today.year(), DEC);
    Serial.print('/');
    Serial.print(today.month(), DEC);
    Serial.print('/');
    Serial.println(today.day(), DEC);

    startTime = DateTime(today.year(),today.month(),today.day(),00,00,0); // TODO: get today() hr=0 min=0
    // 24 hours in a day but 32 segments...
    //(24hr * 60min * 60sec)/32 // seconds/day / 32 
    for( int8_t x=1; x <= iChartCol; x++ ) {
        result = myTideCalc.currentTide(startTime.unixtime() + ((24 * 60L * 60)/iChartCol) * x);
        fChartData[x-1] = result;
        iChartData[x-1] = (int8_t)round(result);
        // test if low tide is less than 0 and move to zero. Disp only does 0-7'
        if( result < 0.50 ) iChartData[x-1] = 0;
        int whole = (int)result;
        //Serial.print(result,3);Serial.print("::");Serial.println((int8_t)abs((result-whole)*100));
        iChartDataDec[x-1] = (int8_t)abs((result-whole)*100);
    }
}

/*
void getMinMaxData() {
    int8_t iFMin, iFMax, iSMin, iSMax;
    iMinMax[0]=iFMin = findMin(0);
    iMinMax[1]=iFMax = findMax(0);      
    if( iFMax < iFMin ) {
        //iFMax = findMax(iFMin);
        iMinMax[1]= iFMax = findMax(iFMin);      
    }
    // find 2nd Min and Max
    iMinMax[2]=iSMin = findMin(iFMax);
    iMinMax[3]=iSMax = findMax(iSMin);      
    if( iSMax < iSMin ) { // 2nd max returned 0
        //iSMax = findMax(iFMin);
      iMinMax[3]=iSMax = findMax(0);      
    }
  
    Serial.print("First Min=");Serial.println(iFMin);
    Serial.print("First Max=");Serial.println(iFMax);
    Serial.print("Second Min=");Serial.println(iSMin);
    Serial.print("Second Max=");Serial.println(iSMax);
}

int8_t findMin(int iStart) {
    for( int i=iStart; i<32; i++ ) {
        if( (i<31) && (fChartData[i]-fChartData[i+1] < 0)) return i;
        return 0;
    }
}

int8_t findMax(int iStart) {
    for( int i=iStart; i<32; i++ ) {
        if( (i<31) && (fChartData[i+1]-fChartData[i] < 0)) return i;
        return 0;
    }
}
*/
