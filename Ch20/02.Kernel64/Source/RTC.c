#include "RTC.h"
#include "Types.h"
#include "AssemblyUtility.h"

// read RTC time
// params:
//   pbHour: address where hour value will be stored
//   pbMinute: address where minute value will be stored
//   pbSecond: address where second value will be stored
// info:
//   this is not thread-safe or process-safe
void kReadRTCTime(BYTE *pbHour, BYTE *pbMinute, BYTE *pbSecond) {
    BYTE bData;

    // read value of hour
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbHour = RTC_BCDTOBINARY(bData);

    // read value of minute
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMinute = RTC_BCDTOBINARY(bData);

    // read value of second
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbSecond = RTC_BCDTOBINARY(bData);
}


// read RTC date
// params:
//   pbYear: address where hour value will be stored
//   pbMonth: address where minute value will be stored
//   pbDayOfMonth: address where 'day of month' value will be stored
//   pbDayOfWeek: address where 'day of week' value will be stored
// info:
//   this is not thread-safe or process-safe
void kReadRTCDate(
    WORD *pbYear,
    BYTE *pbMonth,
    BYTE *pbDayOfMonth,
    BYTE *pbDayOfWeek
) {
    BYTE bData;

    // read value of year
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbYear = RTC_BCDTOBINARY(bData) + 2000;

    // read value of month
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbMonth = RTC_BCDTOBINARY(bData);

    // read value of 'day of month'
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbDayOfMonth = RTC_BCDTOBINARY(bData);

    // read value of 'day of week'
    kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
    bData = kInPortByte(RTC_CMOSDATA);
    *pbDayOfWeek = RTC_BCDTOBINARY(bData);
}


// convert day of week in integer form to string form
// params:
//   bDayOfWeek: 'day of week' value given by CMOS
// return:
//   pointer to string of 'day of week'
// info:
//   the string pointer does not point dynamically allocated memory.
//   you should not modify the memory
char *kConvertDayOfWeekToString(BYTE bDayOfWeek) {
    static char *vpcDayOfWeekString[8] = {
        "Error",
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday"
    };

    if (bDayOfWeek > 7) {
        return vpcDayOfWeekString[0];
    }
    return vpcDayOfWeekString[bDayOfWeek];
}