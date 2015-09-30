#ifndef __MYTIME_H
#define __MYTIME_H

#include <time.h>

int getSec();
int getMinute();
int getHour();
int getMonth();
int getYear();
char *getDayName(time_t when);
struct tm *getLocalTime(time_t *when);

#ifdef WIN32
 #include <windows.h>
 void UnixTimeToFileTime(time_t t, LPFILETIME pft);
 void UnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst);
#endif

#endif
