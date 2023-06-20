/*  Simple test for ebay 128x64 oled run via I2C on a 
    Arduino Pro Mini 3.3V (328P), with ebay DS3231 real time
    clock module.
    
    The sketch will calculate the current tide height for
    the site (assuming clock is set correctly) and display
    the tide height and time on a ssd1306-controller OLED
    128x64 display. Time updates every second, tide updates
    as the last significant digit changes (10-20 seconds). 

*/

#include <Wire.h> 
#include "SSD1306Ascii.h" // https://github.com/greiman/SSD1306Ascii
#include "SSD1306AsciiWire.h" // https://github.com/greiman/SSD1306Ascii
#include <SPI.h> // Required for RTClib to compile properly
#include <RTClib.h> // From https://github.com/millerlp/RTClib
// Real Time Clock setup
RTC_DS3231 RTC; // Uncomment when using this chip
int dS = 0;
DateTime futureHigh;
DateTime futureLow;
DateTime future;
int slope;
int i = 0;
int zag = 0;
bool gate = 1;
float tidalDifference=0;
 float pastResult;
 bool bing = 1;
 bool futureLowGate = 0;
 bool futureHighGate = 0;


// Tide calculation library setup.
// Change the library name here to predict for a different site.
//#include "TidelibSanDiegoSanDiegoBay.h"
#include "TidelibSitkaBaronofIslandSitkaSoundAlaska.h"
// Other sites available at http://github.com/millerlp/Tide_calculator
TideCalc myTideCalc; // Create TideCalc object 

// 0X3C+SA0 - 0x3C or 0x3D for oled screen on I2C bus
#define I2C_ADDRESS 0x3C

SSD1306AsciiWire oled; // create oled dispaly object

long oldmillis; // keep track of time
float results; // tide height
DateTime now; // define variable to hold date and time
// Enter the site name for display. 11 characters max
char siteName[11] = "SITKA";  
//------------------------------------------------------------------------------
void waterLevel(DateTime now, DateTime futureHigh, DateTime futureLow,int dS){
  bool hiLow;
  if( int(futureHigh.unixtime() - futureLow.unixtime()) < 0) hiLow = 1;
  if( int(futureHigh.unixtime() - futureLow.unixtime()) > 0) hiLow = 0;
  float resultsHigh = myTideCalc.currentTide(futureHigh); 
  float resultsLow = myTideCalc.currentTide(futureLow); 
  float resultsNow = myTideCalc.currentTide(now); 
  oled.home();
  oled.set2X();
  oled.clear();
  if(hiLow){
    
    oled.print("HI  ");
    oled.print(resultsHigh, 1);
    oled.println(" ft");
    oled.println("  Rising");
    oled.println();
    oled.print("NOW ");
    oled.print(resultsNow, 1);
    oled.print(" ft");
  }
    else {
    oled.print("NOW  ");
    oled.print(resultsNow, 1);
    oled.println(" ft");
    oled.println();
    oled.println(" Dropping");
   
    oled.print("LOW ");
    oled.print(resultsLow, 1);
    oled.print(" ft");
     
    }
    
  }
void graphTide(DateTime now, DateTime futureHigh, DateTime futureLow,int dS){
  bool hiLow;
  oled.home();
  oled.set2X();
  oled.clear();
  int hourDifference;
   if(futureHigh.unixtime() > futureLow.unixtime() ){
      hourDifference =  ((futureLow.unixtime() - now.unixtime())/3600);
     hiLow = 0;
   }
   else {
     hourDifference =  ((futureHigh.unixtime() - now.unixtime())/3600);
    hiLow = 1;
   }
   
   
   oled.print(hourDifference);
   oled.println(" HOURS TO");
   if(hiLow)oled.println("   HIGH"); 
   else oled.println("    LOW");
   
    oled.set1X();
    oled.println();
    hourDifference = map(hourDifference, 0,6,20,1);
    hourDifference = constrain(hourDifference,1,20);
    for(int printer = 0; printer < 20; printer++) oled.print("=");
    oled.println();
    for(int printer = 0; printer < hourDifference; printer ++) {
    oled.print("I");
    }
    oled.println();
    for(int printer = 0; printer < 20; printer++) oled.print("=");
    
}

void oledScreen(DateTime now, DateTime futureHigh, DateTime futureLow,int dS){
   bool hiLow;
   if( int(futureHigh.unixtime() - futureLow.unixtime()) < 0) hiLow = 1;
   if( int(futureHigh.unixtime() - futureLow.unixtime()) > 0) hiLow = 0;
  
  oled.clear();
  oled.home();
  oled.set2X();
  if (hiLow) {
    oled.print("HI     ");
    oled.println("LOW");
    oled.print(futureHigh.hour() +1, DEC); 
    oled.print(":");
  if (futureHigh.minute() < 10) {
    oled.print("0");
    oled.print(futureHigh.minute());
   }
  else if (futureHigh.minute() >= 10) {
    oled.print(futureHigh.minute());
  }
  oled.print(" ");
    oled.print(futureLow.hour() +1, DEC); 
  oled.print(":");
  if (futureLow.minute() < 10) {
    oled.print("0");
    oled.print(futureLow.minute());
   }
  else if (futureLow.minute() >= 10) {
    oled.print(futureLow.minute());
  }
    
  }
  else {
    oled.print("LOW     ");
    oled.println("HI");
    oled.print(futureLow.hour() +1, DEC); 
  oled.print(":");
  if (futureLow.minute() < 10) {
    oled.print("0");
    oled.print(futureLow.minute());
   }
  else if (futureLow.minute() >= 10) {
    oled.print(futureLow.minute());
  }
  oled.print(" ");
   oled.print(futureHigh.hour() +1, DEC); 
   oled.print(":");
  if (futureHigh.minute() < 10) {
    oled.print("0");
    oled.print(futureHigh.minute());
   }
  else if (futureHigh.minute() >= 10) {
    oled.print(futureHigh.minute());
  }
  }
  oled.println();
  
  
 
results = myTideCalc.currentTide(now); 
      //oled.home();
      oled.set2X();  // Enable large font    
      //oled.println(); // Print site name, move to next line
      oled.print("  ");
      oled.println("SITKA"); // print tide ht. to 3 decimal places
      //oled.println(" ft");
      oled.set1X(); // Enable normal font
      oled.println("  Tide Location");
  oled.print(now.year(), DEC);
  oled.print("/");
  oled.print(now.month(), DEC); 
  oled.print("/");
  oled.print(now.day(), DEC); 
  oled.print("  ");
  oled.print(now.hour() + 1, DEC); 
  oled.print(":");
  if (now.minute() < 10) {
    oled.print("0");
    oled.print(now.minute());
   }
  else if (now.minute() >= 10) {
    oled.print(now.minute());
  }
 oled.print(" ");

}
void setup() {
  Wire.begin();  
  RTC.begin();
  Serial.begin(57600);
  //RTC.adjust(DateTime(F(__DATE__), F(__TIME__))); 
  // Start up the oled display
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.set400kHz();  
  oled.setFont(Adafruit5x7); 
  oled.setScroll(true);   
  oled.clear();
  oled.home();
  oled.set2X();
  
  oldmillis = millis() - 15000;
}
//------------------------------------------------------------------------------
void loop() {
  // The main statement block will run once per second
  if ( oldmillis + 15000 < millis() ) {
      oldmillis = millis(); // update oldmillis
      now = RTC.now(); // update time
      char buf[20]; // declare a string buffer to hold the time result
      // Create a string representation of the date and time, 
      // which will be put into 'buf'. 
      //now.toString(buf, 20); 
      // Now extract the time by making another character pointer that
      // is advanced 10 places into buf to skip over the date. 
      char *subbuf = buf + 10;
      // Calculate current tide height
       if((now.month()>3&&now.month()<11)||(now.month()==3&&now.day()>11)||(now.month()==11&&now.day()<6))dS=1;
   now = (now.unixtime() - dS*3600);
   bing = 1;
   i = 0;
    pastResult=myTideCalc.currentTide(now);
  while(bing){ //This loop asks when the next high or low tide is by checking 15 min intervals from current time
    i++;
   
    DateTime future(now.unixtime() + (i*5*60L));
    results=myTideCalc.currentTide(future);
    tidalDifference=results-pastResult;
    if (gate){
      if(tidalDifference<0)slope=0;//if slope is positive--rising tide--slope neg falling tide
      else slope=1;
      gate=0;
   }
   if(tidalDifference>0&&slope==0){
      futureLow = future;
      gate=1;
      //bing = 0;
      futureLowGate = 1;
   }
    else if(tidalDifference<0&&slope==1){
    futureHigh = future;
    gate=1;
    //bing = 0;
    futureHighGate = 1;
  
   }
   if( futureHighGate && futureLowGate) {
    oledScreen( now, futureHigh, futureLow, dS);
    delay(4000);
    waterLevel( now, futureHigh, futureLow, dS);
    delay(4000);
    graphTide( now, futureHigh, futureLow, dS);
    delay(4000);
    gate = 1;
    bing = 0;
    futureHighGate = 0;
    futureLowGate = 0;
   }
    pastResult=results;
    //Serial.print("results");
    //Serial.print(results);
    //Serial.print(future.year());
    
  }
  
  }
}

