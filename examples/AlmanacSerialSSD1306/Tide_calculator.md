# Tide_calculator
  
Arduino code and associated files for calculating tide height. This assumes that you have downloaded and installed the latest version of the Arduino software (1.8.0 or newer), and have an Arduino with attached real time clock, based on the Maxim DS3231 chip I2C-based chip.
  
## Download

Download [Luke Miller's Tide package](https://github.com/millerlp/Tide_calculator/archive/master.zip)
Extract the ZIP archive and grab the directories you need (detailed below in the Installation section).  

## Installation

To generate a prediction of the current tide height for a site, install the following:

Copy the folder with the library for your site (i.e. TidelibMontereyMontereyHarborCalifornia) into Arduino/libraries/)

Open the Tide_calculator_check example sketch in the Arduino IDE by going to File>Examples>TidelibMontereyHarbor>Tide_calculator_check. The code to call the tide prediction library is referenced in the Initial Setup section, near line 55, with a line like: ```\#include "TidelibMontereyMontereyHarborCalifornia.h"``` That line should contain the name of the library for your local site that you copied into Arduino/libraries/.

Open and run Tide_calculator_check.ino to ensure that the predicted tide levels match your "real" tide tables from a more reliable source. In the Serial Monitor, enter a date and time in the format YYYY MM DD HH MM, with the spaces between the number values. For example, noon on Jan 1, 2019 would be typed into the Serial Monitor as 2019 1 1 12 00. You must set the Serial Monitor to send a 'newline' character when you hit return (see the dropdown menu in the lower right of the Serial Monitor, choose 'newline'). If you are successful, the Arduino should report back the predicted tide height for your date and time, in units of feet above the zero tide line. Keep in mind that this library does not know about daylight savings time, so it assumes your time values are always being entered (and predicted back to you) in local standard time (i.e. the time zone offset for ~November to ~March). If you ask for tide heights during daylight savings time (~March to ~November), the predicted tide height will still be for standard time, and thus it may be one hour off from your tide tables or other software that accounts for the time shift.

The tide calculator doesn't know about local daylight time (LDT) working only on local standard time (LST). Make sure the time you enter is in your local standard time, **NOT** Daylight Savings Time (which runs Mar-Nov in most places). The tide prediction routine relies on the time being set to local standard time for your site, otherwise you won't get the correct current tide height out, instead you'll get the tide for one hour offset.
  
## Generating tide prediction libaries for other sites  
  
If there is no folder containing a tide prediction library for your desired site, it will be necessary to generate a library using the R scripts found in the Generate_new_site_libraries directory. The harmonic data for NOAA sites are all in the Harmonics_20181227.Rdata file. With these data, you can generate a new library by running the R script tide_harmonics_library_generator.R. Inside that file, you must enter a name for the site you're interested in on the line
  
stationID = 'Monterey Harbor'
  
Change the value inside the quote marks to match your site, save the file, and run the script in R. It will create a new directory with the necessary library files inside that can be copied into the Arduino/libraries/ folder. Find the name for your site by looking through the [XTide website](http://www.flaterco.com/xtide/locations.html)  
  
XTide only produces harmonics for ~865 reference tide stations (all others are approximated from these tide stations), so you need to find the nearest station listed as "Ref" on that page, rather than "Sub" stations.

The other R scripts in the Generate_new_site_libraries directory could be used to make an updated Rdata file when XTide updates the original harmonics database and after you convert it to a text file using the libtcd library from the XTide site (see the notes inside
the R scripts).
