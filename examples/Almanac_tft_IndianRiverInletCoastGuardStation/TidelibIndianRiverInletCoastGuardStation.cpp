/*  TidelibIndianRiverInlet(CoastGuardStation).cpp 
 This source file contains a tide calculation function for the site listed
 below. This file and the associated header file should be placed in the
 Ardiuno/libraries/ directory inside a single folder.
 Luke Miller, 2023-07-04 
 http://github.com/millerlp/Tide_calculator
 Released under the GPL version 3 license.
 Compiled for Arduino v1.6.4 circa 2015

 The harmonic constituents used here were originally derived from 
 the Center for Operational Oceanic Products and Services (CO-OPS),
 National Ocean Service (NOS), National Oceanic and Atmospheric 
 Administration, U.S.A.
 The data were originally processed by David Flater for use with XTide,
 available at http://www.flaterco.com/xtide/files.html
 The predictions from this program should not be used for navigation
 and no accuracy or warranty is given or implied for these tide predictions.
 */
#include <Arduino.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include "RTClib.h" // https://github.com/millerlp/RTClib
#include "TidelibIndianRiverInletCoastGuardStation.h"

unsigned int YearIndx = 0; // Used to index rows in the Equilarg/Nodefactor arrays
float currHours = 0;          // Elapsed hours since start of year
const int adjustGMT = 5;     // Time zone adjustment to get time in GMT.
//Make sure this is correct for the local standard time of the tide station.
// 8 = Pacific Standard Time (America/Los_Angeles)
/* Initialize harmonic constituent arrays. These each hold 37 values for
the tide site that was extracted using the R scripts:
tide_harmonics_parse.R and tide_harmonics_library_generator.R

The values are available from NOAA's http://tidesandcurrent.noaa.gov site.
Kappa here is referred to as 'Phase' on NOAA's site. The order of the
constituents is shown below in the names. Unfortunately this does not match
NOAA's order, so you will have to rearrange NOAA's values if you want to
put new site values in here by hand.
The Speed, Equilarg and Nodefactor arrays can all stay the same for any site.
*/

// Selected station: Inlet (Coast Guard Station), Indian River, Delaware
char stationID[] = "Inlet (Coast Guard Station), Indian River, Delaware";
// Selection station ID number: 8558690
const long stationIDnumber = 8558690;
// The 'datum' printed here is the difference between mean sea level and 
// mean lower low water for the NOAA station. These two values can be 
// found for NOAA tide reference stations on the tidesandcurrents.noaa.gov
//  site under the datum page for each station.
const float Datum = 1.48 ; // units in feet
// Harmonic constant names: J1, K1, K2, L2, M1, M2, M3, M4, M6, M8, N2, 2N2, O1, OO1, P1, Q1, 2Q1, R2, S1, S2, S4, S6, T2, LDA2, MU2, NU2, RHO1, MK3, 2MK3, MN4, MS4, 2SM2, MF, MSF, MM, SA, SSA
// These names match the NOAA names, except LDA2 here is LAM2 on NOAA's site
typedef float PROGMEM prog_float_t; // Need to define this type before use
// Amp is the amplitude of each of the harmonic constituents for this site
const prog_float_t Amp[] PROGMEM = {0.02,0.23,0.06,0.05,0.01,1.23,0,0.07,0.01,0,0.28,0.03,0.2,0.01,0.08,0.03,0.01,0,0.02,0.22,0,0,0.01,0.01,0.02,0.05,0.01,0.02,0.02,0.04,0.02,0,0.05,0,0.06,0.2,0.1};
// Kappa is the 'modified' or 'adapted' phase lag (Epoch) of each of the 
// harmonic constituents for this site.
const prog_float_t Kappa[] PROGMEM = {216.4,210.6,42.6,43.9,204.6,24.1,0,179.7,248.7,0,1.9,349.3,198.9,222.2,203.9,193.4,187.1,0,122.9,47.5,0,0,47.4,35,354.2,4.6,193.8,17.1,13.1,165.5,203.3,0,182.2,0,243.6,153.5,43.6};
// Speed is the frequency of the constituent, denoted as little 'a' by Hicks 2006
const prog_float_t Speed[] PROGMEM = {15.58544,15.04107,30.08214,29.52848,14.49669,28.9841,43.47616,57.96821,86.95231,115.9364,28.43973,27.89535,13.94304,16.1391,14.95893,13.39866,12.85429,30.04107,15,30,60,90,29.95893,29.45563,27.96821,28.51258,13.47151,44.02517,42.92714,57.42383,58.9841,31.0159,1.098033,1.015896,0.5443747,0.0410686,0.0821373};
const prog_float_t Equilarg[10][37] PROGMEM = { 
{92.29,6.43,192.41,74.18,317.02,146.39,219.58,292.78,79.17,225.56,58.93,331.47,141.56,48.09,349.6,54.1,326.64,177.06,180,0,0,0,2.94,91.44,293.87,21.33,16.51,152.82,286.35,205.32,146.39,213.61,43.26,213.61,87.46,280.4,200.79},
{184.25,8.66,197.15,246.07,213.12,247.82,191.73,135.64,23.46,271.27,71.64,255.45,239.75,316.39,349.84,63.57,247.38,176.8,180,0,0,0,3.2,3.4,136.06,312.24,304.17,256.48,126.97,319.45,247.82,112.18,308.32,112.18,176.18,280.16,200.32},
{290.4,12,204.13,41.02,96.21,324.92,127.37,289.83,254.75,219.66,46.94,128.97,312.48,252.38,349.1,34.51,116.54,177.53,180,0,0,0,2.47,262.34,289.52,207.49,195.06,336.91,277.83,11.86,324.92,35.08,239.95,35.08,277.97,280.9,201.81},
{22.4,14.26,208.93,250.96,25.28,66.36,99.53,132.71,199.07,265.43,59.66,52.97,50.65,160.77,349.33,43.96,37.26,177.28,180,0,0,0,2.72,174.3,131.72,138.41,122.7,80.62,118.45,126.02,66.36,293.64,145.06,293.64,6.69,280.67,201.33},
{114.06,16.25,213.08,101.09,323.67,167.68,71.52,335.37,143.05,310.73,72.27,336.85,149.03,68.26,349.57,53.62,318.2,177.02,180,0,0,0,2.98,86.15,333.8,69.21,50.56,183.94,319.11,239.95,167.68,192.32,49.61,192.32,95.42,280.43,200.85},
{205.14,17.78,216.09,279.46,220.94,268.83,43.24,177.66,86.49,355.32,84.69,260.55,247.83,334.17,349.81,63.69,239.55,176.77,180,0,0,0,3.23,357.82,175.7,359.84,338.83,286.61,159.88,353.52,268.83,91.17,313.17,91.17,184.14,280.19,200.38},
{309.38,19.6,219.48,75.08,101.48,345.37,338.06,330.74,316.11,301.48,59.44,133.52,321.92,264.98,349.06,35.99,110.07,177.5,180,0,0,0,2.5,256.21,328.61,254.54,231.08,4.97,311.14,44.82,345.37,14.63,241.53,14.63,285.93,280.94,201.87},
{38.35,19.55,218.92,275.51,16.35,86.05,309.08,172.1,258.16,344.21,71.4,56.75,62.35,165.06,349.3,47.7,33.05,177.24,180,0,0,0,2.76,167.41,170.04,184.69,160.99,105.6,152.56,157.45,86.05,273.95,141.35,273.95,14.65,280.7,201.39},
{125.79,18.44,216.22,118.09,318.69,186.5,279.75,13.01,199.51,26.01,83.13,339.76,164.08,60.76,349.54,60.71,317.34,176.98,180,0,0,0,3.02,78.39,11.25,114.62,92.2,204.94,354.57,269.63,186.5,173.5,38.34,173.5,103.37,280.46,200.92},
{211.48,16.21,211.57,307.74,220.29,286.76,250.14,213.51,140.27,67.03,94.66,262.57,267.37,311.4,349.78,75.28,243.18,176.73,180,0,0,0,3.27,349.17,212.25,44.35,24.96,302.97,197.3,21.42,286.76,73.24,292.02,73.24,192.09,280.22,200.44} 
 };

const prog_float_t Nodefactor[10][37] PROGMEM = { 
{1.1492,1.1009,1.2736,1.0393,1.7582,0.9683,0.9528,0.9376,0.9079,0.8791,0.9683,0.9683,1.1631,1.6753,1,1.1631,1.1631,1,1,1,1,1,1,0.9683,0.9683,0.9683,1.1631,1.066,1.0322,0.9376,0.9683,0.9683,1.3967,0.9683,0.8888,1,1},
{1.163,1.1112,1.3104,0.5914,2.2831,0.9639,0.9464,0.9291,0.8956,0.8633,0.9639,0.9639,1.1801,1.7658,1,1.1801,1.1801,1,1,1,1,1,1,0.9639,0.9639,0.9639,1.1801,1.0711,1.0324,0.9291,0.9639,0.9639,1.4444,0.9639,0.8738,1,1},
{1.164,1.112,1.3131,1.0008,1.8466,0.9636,0.9459,0.9285,0.8947,0.8621,0.9636,0.9636,1.1814,1.7726,1,1.1814,1.1814,1,1,1,1,1,1,0.9636,0.9636,0.9636,1.1814,1.0715,1.0324,0.9285,0.9636,0.9636,1.4479,0.9636,0.8727,1,1},
{1.1522,1.1031,1.2815,1.3288,1.0994,0.9674,0.9515,0.9358,0.9052,0.8757,0.9674,0.9674,1.1668,1.6946,1,1.1668,1.1668,1,1,1,1,1,1,0.9674,0.9674,0.9674,1.1668,1.0671,1.0323,0.9358,0.9674,0.9674,1.4069,0.9674,0.8856,1,1},
{1.1276,1.0849,1.2196,1.091,1.6268,0.9748,0.9625,0.9502,0.9263,0.903,0.9748,0.9748,1.137,1.544,1,1.137,1.137,1,1,1,1,1,1,0.9748,0.9748,0.9748,1.137,1.0576,1.0309,0.9502,0.9748,0.9748,1.3257,0.9748,0.9111,1,1},
{1.0904,1.0578,1.1347,0.6859,2.1026,0.9852,0.9779,0.9705,0.9561,0.942,0.9852,0.9852,1.093,1.3431,1,1.093,1.093,1,1,1,1,1,1,0.9852,0.9852,0.9852,1.093,1.0422,1.0267,0.9705,0.9852,0.9852,1.2123,0.9852,0.9468,1,1},
{1.042,1.0235,1.0379,0.9408,1.7337,0.9973,0.9959,0.9945,0.9918,0.9891,0.9973,0.9973,1.0374,1.1206,1,1.0374,1.0374,1,1,1,1,1,1,0.9973,0.9973,0.9973,1.0374,1.0207,1.0179,0.9945,0.9973,0.9973,1.0788,0.9973,0.9887,1,1},
{0.9854,0.9845,0.9416,1.2309,0.9912,1.0098,1.0147,1.0196,1.0296,1.0396,1.0098,1.0098,0.9745,0.9059,1,0.9745,0.9745,1,1,1,1,1,1,1.0098,1.0098,1.0098,0.9745,0.9942,1.0039,1.0196,1.0098,1.0098,0.9401,1.0098,1.0323,1,1},
{0.9266,0.9453,0.8573,1.1617,1.0896,1.0212,1.0321,1.0429,1.0651,1.0877,1.0212,1.0212,0.9107,0.7229,1,0.9107,0.9107,1,1,1,1,1,1,1.0212,1.0212,1.0212,0.9107,0.9654,0.9859,1.0429,1.0212,1.0212,0.8118,1.0212,1.0726,1,1},
{0.8739,0.9113,0.7941,0.9272,1.5305,1.0304,1.046,1.0617,1.094,1.1273,1.0304,1.0304,0.8549,0.5873,1,0.8549,0.8549,1,1,1,1,1,1,1.0304,1.0304,1.0304,0.8549,0.939,0.9675,1.0617,1.0304,1.0304,0.709,1.0304,1.1049,1,1} 
 };

// Define unix time values for the start of each year.
//                                      2023       2024       2025       2026       2027       2028       2029       2030       2031       2032
const unsigned long startSecs[] PROGMEM = {1672531200,1704067200,1735689600,1767225600,1798761600,1830297600,1861920000,1893456000,1924992000,1956528000};

// 1st year of data in the Equilarg/Nodefactor/startSecs arrays.
const unsigned int startYear = 2023;
//------------------------------------------------------------------
// Define some variables that will hold extract values from the arrays above
float currAmp, currSpeed, currNodefactor, currEquilarg, currKappa, tideHeight;

// Constructor function, doesn't do anything special
TideCalc::TideCalc(void){}

// Return tide station name
char* TideCalc::returnStationID(void){
    return stationID;
}

// Return NOAA tide station ID number
long TideCalc::returnStationIDnumber(void){
    return stationIDnumber;
}

// currentTide calculation function, takes a DateTime object from real time clock
float TideCalc::currentTide(DateTime now) {
	// Calculate difference between current year and starting year.
	YearIndx = now.year() - startYear;
 	// Calculate hours since start of current year. Hours = seconds / 3600
	currHours = (now.unixtime() - pgm_read_dword_near(&startSecs[YearIndx])) / float(3600);
   // Shift currHours to Greenwich Mean Time
   currHours = currHours + adjustGMT;
   // *****************Calculate current tide height*************
   tideHeight = Datum; // initialize results variable, units of feet.
   for (int harms = 0; harms < 37; harms++) {
       // Step through each harmonic constituent, extract the relevant
       // values of Nodefactor, Amplitude, Equilibrium argument, Kappa
       // and Speed.
       currNodefactor = pgm_read_float_near(&Nodefactor[YearIndx][harms]);
 		currAmp = pgm_read_float_near(&Amp[harms]);
       currEquilarg = pgm_read_float_near(&Equilarg[YearIndx][harms]);
       currKappa = pgm_read_float_near(&Kappa[harms]);
       currSpeed = pgm_read_float_near(&Speed[harms]);
    // Calculate each component of the overall tide equation
    // The currHours value is assumed to be in hours from the start of the
    // year, in the Greenwich Mean Time zone, not the local time zone.
       tideHeight = tideHeight + (currNodefactor * currAmp *
           cos( (currSpeed * currHours + currEquilarg - currKappa) * DEG_TO_RAD));
    }
    //******************End of Tide Height calculation*************
    return tideHeight;  // Output of tideCalc is the tide height, units of feet
}
