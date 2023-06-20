#ifndef  TidelibClearwaterBeachGulfOfMexicoFlorida_h 
#define  TidelibClearwaterBeachGulfOfMexicoFlorida_h 

class TideCalc {
 public:
	    TideCalc();
    float currentTide(DateTime now);
    char* returnStationID(void);
    long returnStationIDnumber(void);
};
#endif
