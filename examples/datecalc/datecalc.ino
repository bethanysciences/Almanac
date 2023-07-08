#include "RTClib.h"
RTC_DS3231 rtc;

void setup () {
    Serial.begin(57600);
    rtc.begin();
    if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    DateTime now = rtc.now();
    DateTime dt1 = now;
    // copy dt1 (dt1.unixtime() + 300);
    
    // dt1.unixtime() += 300;
    dt1 = dt1 + TimeSpan(300);

    DateTime dt2 = dt1 + TimeSpan(0, 0, 5, 0);

    char now_time_buf[]  = "    now YYYY/MM/DD hh:mm:ss";
    char new_time_buf[]  = "dt1 now YYYY/MM/DD hh:mm:ss";
    char span_time_buf[] = "dt2 +5m YYYY/MM/DD hh:mm:ss";

    Serial.println(now.toString(now_time_buf));
    Serial.println(dt1.toString(new_time_buf));
    Serial.println(dt2.toString(span_time_buf));
}

void loop () {
}
