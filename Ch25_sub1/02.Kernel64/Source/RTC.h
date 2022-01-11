#ifndef __RTC_H__
#define __RTC_H__

#include "Types.h"


/* RTC I/O ports */

#define RTC_CMOSADDRESS     0x70
#define RTC_CMOSDATA        0x71


/* CMOS memory address */

#define RTC_ADDRESS_SECOND      0x00
#define RTC_ADDRESS_MINUTE      0x02
#define RTC_ADDRESS_HOUR        0x04
#define RTC_ADDRESS_DAYOFWEEK   0x06
#define RTC_ADDRESS_DAYOFMONTH  0x07
#define RTC_ADDRESS_MONTH       0x08
#define RTC_ADDRESS_YEAR        0x09


/* conversion macro */

//  macro that convert BCD to binary
#define RTC_BCDTOBINARY(x) ((((x) >> 4) * 10) + ((x) & 0x0F))


/* RTC related functions */

// read RTC time
// params:
//   pbHour: address where hour value will be stored
//   pbMinute: address where minute value will be stored
//   pbSecond: address where second value will be stored
void kReadRTCTime(BYTE *pbHour, BYTE *pbMinute, BYTE *pbSecond);


// read RTC date
// params:
//   pbYear: address where hour value will be stored
//   pbMonth: address where minute value will be stored
//   pbDayOfMonth: address where 'day of month' value will be stored
//   pbDayOfWeek: address where 'day of week' value will be stored
void kReadRTCDate(
    WORD *pbYear,
    BYTE *pbMonth,
    BYTE *pbDayOfMonth,
    BYTE *pbDayOfWeek
);


// convert day of week in integer form to string form
// params:
//   bDayOfWeek: 'day of week' value given by CMOS
// return:
//   pointer to string of 'day of week'
// info:
//   the string pointer does not point dynamically allocated memory.
//   you should not modify the memory
char *kConvertDayOfWeekToString(BYTE bDayOfWeek);

#endif /* __RTC_H__ */