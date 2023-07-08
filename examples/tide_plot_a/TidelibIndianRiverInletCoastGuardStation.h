#ifndef  TidelibIndianRiverInlet(CoastGuardStation)_h 
#define  TidelibIndianRiverInlet(CoastGuardStation)_h 
#include "RTClib.h"

class TideCalc {
 public:
	 TideCalc();
    float currentTide(DateTime now);
    char* returnStationID(void);
    long returnStationIDnumber(void);
};
#endif