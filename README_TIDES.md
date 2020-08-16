# Tide Calculator

Arduino code and associated files for calculating tide height. This assumes that you have downloaded and installed the latest version of the Arduino software (1.6.4 or newer), and have an Arduino with attached DS1307 or DS3231 I2C-based real time clock (RTC)

## Installation

To generate a prediction of the current tide height for a site, install the following:

1. Install the [RTClib](https://github.com/millerlp/RTClib) RTC library
2. Copy the folder with the library for your site (i.e. TidelibMontereyMontereyHarborCalifornia) into Arduino/libraries/ directory
3. Run Examples\Tide_calculator_check.ino to verify predicted tide levels match your "real" tide tables from a more reliable source

## Setting a DS1307 or DS3231 Real Time Clock

The RTC must be set to Local Standard Time (LST) to produce proper readings. Convert time to Local Daylight Time (LDT) if during dalyight savings time

## Generating tide prediction libaries for other sites

If there is no folder containing a tide prediction library for your desired site, it will be necessary to generate a library using the R scripts found in the  Generate_new_site_libraries directory. The harmonic data for NOAA sites are all in the Harmonics_20181227.Rdata file. With these data, you can generate a new library by running the R script tide_harmonics_library_generator.R. Inside that file, you must enter a name for the site you're interested in on the line stationID = 'Monterey Harbor'

Change the value inside the quote marks to match your site, save the file, and run the script in R. It will create a new directory with the necessary library files inside that
can be copied into the Arduino/libraries/ folder. Find the name for your site by looking through the [XTide](http://www.flaterco.com/xtide/locations.html) XTide only produces harmonics for ~865 reference tide stations (all others are approximated from these tide stations), so you need to find the nearest station listed as "Ref" on that page, rather than "Sub" stations.

The other R scripts in the Generate_new_site_libraries directory could be used to make an updated Rdata file when XTide updates the original harmonics database and after you convert it to a text file using the libtcd library from the XTide site (see the notes inside the R scripts).
