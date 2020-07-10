
/* 
 * -------------------------------------------------------------------------------------------------------------------------------------
 * 
 * Adafruit Feather M4 Microchip SAMD51
 * https://learn.adafruit.com/adafruit-feather-m4-express-atsamd51
 * 
 * 
 * -------------------------------------------------------------------------------------------------------------------------------------
 * 
 * Sparkfun AS3935 Franklin Lightning Detector Brakout V2
 * https://learn.sparkfun.com/tutorials/sparkfun-as3935-lightning-detector-hookup-guide-v20
 * distanceToStorm()                            strike distance in kilometers x .621371 for miles
 * clearStatistics()                            zero distance registers   
 * resetSettings()                              resets to defaults
 * setNoiseLevel(int)                           read/set noise floor 1-7 (2 default)
 * readNoiseLevel()
 * maskDisturber(bool)                          read/set mask disturber events (false default)
 * readMaskDisturber(bool)
 * setIndoorOutdoor(bin 0x12, 0xE)              0x12 (default) attunates sensitivity for inside
 * readIndoorOutdoor(bin 0x12, 0xE)
 * watchdogThreshold(int)                       read/set watchdog threshold 1-10 (2 default)
 * readWatchdogThreshold()
 * spikeRejection(int)                          read/set spike reject 1-11 (2 default)
 * readSpikeRejection()
 * lightningThreshold(int)                      read/set # strikes interrupt threshold 1,5,9, or 26
 * readLightningThreshold()
 * lightningEnergy(long)                        strike energy
 * lightning.powerDown()                        wake after power down internal oscillators will be
 * lightning.wakeUp()                           recalibrated to default antenna resonance frequency
 *                                              (500kHz) skewing built-up calibrations. Calibrate 
 *                                              antenna before using this function
 * 
 * 
 * -------------------------------------------------------------------------------------------------------------------------------------
 * 
 * Waveshare Display 2.9" 128x298 e-paper display                                             
 * using GxEPD2 Library By Jean-Marc Zingg https://github.com/ZinggJM/GxEPD2
 * 
 * ENABLE_GxEPD2_GFX 1 uncomment GxEPD2_GFX.h or include GFX.h before any include GxEPD2_GFX.h
 * 
 * init(uint32_t serial_diag_bitrate = 0)
 * 
 * fillScreen(color)                                                    GxEPD_BLACK, GxEPD_WHITE, GxEPD_RED
 * display(bool partial_update_mode = false)                            display buffer content to screen, useful for full screen buffer
 * displayWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
 * 
 * drawPixel(int16_t x, int16_t y, uint16_t color)
 * 
 * setPartialWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)     y and h should be multiple of 8, for rotation 1 or 3
 *                                                                      x and w should be multiple of 8, for rotation 0 or 2
 * displayWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)        display part of buffer content to screen
 * 
 * setFullWindow()
 * 
 * Adafruit_GFX Functions
 * drawRoundRect(x0, y0, w, h, radius, color);
 * 
 */

#include <SPI.h>
#include <Wire.h>
#include <bittable.h>
#include <pins.h>


// ------------  WAVESHARE 2.9" E-PAPER DISPLAY  ------------ //

#define ENABLE_GxEPD2_GFX           0     
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#define WAVE_CS                     10                      // constructors for Adafruit Feather M4
#define WAVE_DC                     9
#define WAVE_RESET                  6
#define WAVE_BUSY                   5
#define MAX_DISPLAY_BUFFER_SIZE 15000ul
#define MAX_HEIGHT_3C(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))

GxEPD2_3C<GxEPD2_290c, MAX_HEIGHT_3C(GxEPD2_290c)> display(GxEPD2_290c(WAVE_CS, WAVE_DC, WAVE_RESET, WAVE_BUSY));
#define ROTATION                    1                       // 0 = vert up, 1 = horiz up, 2 = vert dn, 3 = horiz dn


// ------------ AS3935  ------------ //
#include "SparkFun_AS3935.h"
#define LIGHTNING_INT           0x08
#define DISTURBER_INT           0x04
#define NOISE_INT               0x01
#define AS3935_CS               11                              // AS3935 chip select pin
#define AS3935_STRIKE           4                               // strike detection interrupt pin (HIGH)
byte noiseFloor = 2;
byte watchDogVal = 2;
byte spike = 2;
byte lightningThresh = 1;
SparkFun_AS3935 strike;


// ------------  NEO PIXELS  ------------ //

#include <Adafruit_NeoPixel_ZeroDMA.h>
#define NEO_PIN         8
#define NEO_PIXELS      1
Adafruit_NeoPixel_ZeroDMA strip(NEO_PIXELS, NEO_PIN, NEO_GRB);


// ------------  MELODIC TONES  ------------ //

#include "toneNotes.h"
#define TONE_PIN        A1

// ------------  SETUP  ------------ //

void setup() {
    Serial.begin(115200);                                       // while(!Serial);
    Serial.println("\n\nstartup");

    pinMode(AS3935_STRIKE, INPUT);                              // strike detected int pin set
    SPI.begin();
    while(!strike.beginSPI(AS3935_CS, 2000000));
    // strike.maskDisturber(true);                                 // default = false

    display.init();                                             // init waveshare and send diags to serial print

//    startupMelody();
    introScreen();
    cycleNEO();
    statusScreen();
    batterySprite();
}



void loop() {
    if(digitalRead(AS3935_STRIKE) == HIGH) eventDetected();
//    delay(500);
}



void eventDetected() {
    Serial.print("event register 0x0");
    
    int intVal = strike.readInterruptReg();
    Serial.print(intVal, HEX);
    
    if(intVal == NOISE_INT) {
        Serial.println("\tnoise event");
        strip.setPixelColor(0, 255, 255, 0);
    }
    else if(intVal == DISTURBER_INT) {
        Serial.println("\tdisturber event");
        strip.setPixelColor(0, 255, 0, 255);
    }
    else if(intVal == LIGHTNING_INT) {
        Serial.print("\tstrike distance ");           
        Serial.print(strike.distanceToStorm() /.621371);         //  * .621371 for miles
        Serial.println(" mi."); 
        strip.setPixelColor(0, 0, 0, 255);
    }
    strip.show();
}



void cycleNEO() {
    Serial.print("cycleNEO\t");

    // --- Cycle NEO Pixel ---
    strip.begin();
    strip.setBrightness(32);
    strip.setPixelColor(0, 255, 0, 0); strip.show(); delay(100);
    Serial.print("red ");
    strip.setPixelColor(0, 0, 255, 0); strip.show(); delay(100);
    Serial.print("green ");
    strip.setPixelColor(0, 0, 0, 255); strip.show(); delay(100);
    Serial.print("blue ");
    strip.setPixelColor(0, 255, 255, 255); strip.show();
    Serial.println("white ");
}

void statusScreen() {
    Serial.print("statusScreen\t");
    
    // --- Get AS3935 Paramters ---
    char disturberMask[2][20]   = {"ON", "OFF"};
    char inOutdoor[2][20]       = {"IN", "OUT"};
    int inOut;
    char buf01[40];

    if (strike.readIndoorOutdoor() == 18) inOut = 0;
    else inOut = 1;

    sprintf(buf01, "dist %s  %s  noise %d  watch %d  reg %d  strks %d", 
            disturberMask[strike.readMaskDisturber()], inOutdoor[inOut], 
            strike.readNoiseLevel(), strike.readWatchdogThreshold(),
            strike.readSpikeRejection(), strike.readLightningThreshold());

    
    // --- Display Status ---
    // display.setFont(&FreeSans9pt7b);
    display.setFont();
    display.setTextColor(GxEPD_BLACK);
    display.setRotation(1);
    
    display.setPartialWindow(0, 110, 296, 17);
    display.firstPage();
    do {
        display.drawRoundRect(1, 110, 294, 17, 4, GxEPD_BLACK);
        display.setCursor(8, 116);
        display.print(buf01);
        display.drawFastVLine(60, 110, 17, GxEPD_BLACK);
        display.drawFastVLine(85, 110, 17, GxEPD_BLACK);
        display.drawFastVLine(140, 110, 17, GxEPD_BLACK);
        display.drawFastVLine(195, 110, 17, GxEPD_BLACK);    
        display.drawFastVLine(235, 110, 17, GxEPD_BLACK);    
    }
    while (display.nextPage());
    Serial.println(buf01);
}

void batterySprite() {
    Serial.print("batterySprite\t");

    #define VBATPIN         A6
    
    float batteryVolts = analogRead(VBATPIN);
    batteryVolts *= 2;
    batteryVolts *= 3.3;
    batteryVolts /= 1024;
    
    #define BATTTEXT_STARTX     200                             //  Battery Text X Position
    #define BATTTEXT_STARTY     1                               //  Battery Text Y Position
    #define BATTICON_STARTX     265                             //  Battery Icon X Position
    #define BATTICON_STARTY     1                               //  Battery Icon X Position
    #define BATTICON_WIDTH      30                              //  Battery Icon Width
    #define BATTICON_BARWIDTH3  ((BATTICON_WIDTH - 6) / 3)      //  Battery Icon Bar Width

    char buf0[15];
    sprintf(buf0, "batt %d.%02dv", (int)batteryVolts, (int)(batteryVolts*100)%100);

    display.setFont();
    display.setTextColor(GxEPD_BLACK);
    display.setRotation(1);

    display.setPartialWindow(/*x start*/ 200, /*y start*/ 0, /*width*/ 96, /*height*/ 16);   // y and height should be multiple of 8
    display.firstPage();
    
    do {    
        display.setCursor(BATTTEXT_STARTX, BATTTEXT_STARTY);
        display.print(buf0);

        display.drawLine( BATTICON_STARTX + 1, BATTICON_STARTY,     BATTICON_STARTX + 
                                               BATTICON_WIDTH - 4,  BATTICON_STARTY,     GxEPD_BLACK);
        display.drawLine( BATTICON_STARTX,     BATTICON_STARTY + 1, BATTICON_STARTX, 
                                                                    BATTICON_STARTY + 5, GxEPD_BLACK);
        display.drawLine( BATTICON_STARTX + 1, BATTICON_STARTY + 6, BATTICON_STARTX + 
                                               BATTICON_WIDTH - 4,  BATTICON_STARTY + 6, GxEPD_BLACK);
        display.drawPixel(BATTICON_STARTX +    BATTICON_WIDTH - 3,  BATTICON_STARTY + 1, GxEPD_BLACK);
        display.drawPixel(BATTICON_STARTX +    BATTICON_WIDTH - 2,  BATTICON_STARTY + 1, GxEPD_BLACK);
        display.drawLine( BATTICON_STARTX +    BATTICON_WIDTH - 1,  BATTICON_STARTY + 2, 
                          BATTICON_STARTX +    BATTICON_WIDTH - 1,  BATTICON_STARTY + 4, GxEPD_BLACK);
        display.drawPixel(BATTICON_STARTX +    BATTICON_WIDTH - 2,  BATTICON_STARTY + 5, GxEPD_BLACK);
        display.drawPixel(BATTICON_STARTX +    BATTICON_WIDTH - 3,  BATTICON_STARTY + 5, GxEPD_BLACK);
        display.drawPixel(BATTICON_STARTX +    BATTICON_WIDTH - 3,  BATTICON_STARTY + 6, GxEPD_BLACK);
    
        if (batteryVolts > 4.26F) 
            display.fillRect(BATTICON_STARTX + 2, BATTICON_STARTY + 2, BATTICON_BARWIDTH3 * 3, 3, GxEPD_WHITE);
        else if ((batteryVolts <= 4.26F) && (batteryVolts >= 4.1F)) {
            for (uint8_t i = 0; i < 3; i++) {
                display.fillRect(BATTICON_STARTX + 2 + (i * BATTICON_BARWIDTH3), BATTICON_STARTY + 2, BATTICON_BARWIDTH3 - 1, 3, GxEPD_BLACK);
            }
        }
        else if ((batteryVolts < 4.1F) && (batteryVolts >= 3.8F)) {
            for (uint8_t i = 0; i < 2; i++) {
                display.fillRect(BATTICON_STARTX + 2 + (i * BATTICON_BARWIDTH3), BATTICON_STARTY + 2, BATTICON_BARWIDTH3 - 1, 3, GxEPD_RED);
            }
        }
        else if ((batteryVolts < 3.8F) && (batteryVolts >= 3.4F)) {
            display.fillRect(BATTICON_STARTX + 2, BATTICON_STARTY + 2, BATTICON_BARWIDTH3 - 1, 3, GxEPD_RED);
        }
    }
    while (display.nextPage());
    Serial.println(buf0);
}
    
void startupMelody() {
    Serial.print("startupMelody\t");
    
    int melody[]        = { NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4 };
    int noteDurations[] = {     4,      8,      8,        4,       4,    4,    4,       4 };    // 4 = qtr note, 8 = 8th note
    for (int thisNote = 0; thisNote < 8; thisNote++) {
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(TONE_PIN, melody[thisNote], noteDuration);
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
        Serial.print(thisNote);
        noTone(TONE_PIN);
    }
    Serial.println("done");
}

void introScreen() {
    Serial.println("introScreen()");
        
    display.setFont(&FreeSans12pt7b);
    display.setRotation(1);
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_RED);
    int16_t tbx, tby; uint16_t tbw, tbh;
    const char Start[] = "Strike Detector";
    display.getTextBounds(Start, 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 2) - tby;
    display.setFullWindow();
    display.firstPage();
    do {
        display.setCursor(x, y);
        display.print(Start);
    }
    while (display.nextPage());
}
