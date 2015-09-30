#include "mytime.h"
#include <time.h>
#include <stdlib.h>
#ifdef WIN32
  #include <windows.h>
#endif
#include "debug.h"

struct tm *getLocalTime(time_t *when)
{
  struct tm *mytime;
  int free_when = 0;
  
  if (when == NULL)
  {
    when = malloc(sizeof(time_t));
    *when = time((time_t *)NULL);
    free_when = 1;
  }

#ifndef WIN32
  mytime = (struct tm *)malloc(sizeof(struct tm));
  localtime_r(when, mytime);
#else
  mytime = localtime(when);
#endif
  if (mytime == NULL)
  {
     _ERROR("Memory allocation failed.");
     exit(1);
  }
  
  if (free_when)
    free(when);
  return mytime;
}

int getHour()
{
  struct tm *mytime;
  int hour = 0;

  mytime = getLocalTime((time_t *)NULL);
  hour = mytime->tm_hour;
#ifndef WIN32
  free(mytime);
#endif
  return hour;
}

#ifndef WIN32
char *getDayName(time_t when)
{
  struct tm *mytime;
  char *dayName;

  mytime = getLocalTime(&when);
  dayName = (char *)calloc(20, 1); /* Large vision! */
  strftime(dayName, 20, "%A", mytime);
  free(mytime);
  return dayName;
}
#else
void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
  LONGLONG ll;

  ll = Int32x32To64(t, (long long)10000000LL);
  ll += (long long int)116444736000000000LL;
  pft->dwLowDateTime = (DWORD)ll;
  pft->dwHighDateTime = ll >> 32;
}

void UnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst)
{
  FILETIME ft;

  UnixTimeToFileTime(t, &ft);
  FileTimeToSystemTime(&ft, pst);
}

char *getDayName(time_t when)
{
  SYSTEMTIME Time;
  char *dayName;
  
  /* GetLocalTime(&Time); */
  UnixTimeToSystemTime(when, &Time);
  dayName = (char *)malloc(20);
  if (GetDateFormat(LOCALE_USER_DEFAULT, 0, &Time, "dddd", dayName, 20) == 0)
  {
    printf("Error in GetDateFormat.\n");
    return NULL;
  }
  return dayName;
}
#endif

int getSec()
{
  struct tm *mytime;
  int sec = 0;

  mytime = getLocalTime((time_t *)NULL);
  sec = mytime->tm_sec;
#ifndef WIN32
  free(mytime);
#endif
  return sec;
}

int getMinute()
{
  struct tm *mytime;
  int min = 0;

  mytime = getLocalTime((time_t *)NULL);
  min = mytime->tm_min;
#ifndef WIN32
  free(mytime);
#endif
  return min;
}

int getYear()
{
  struct tm *mytime;
  int year = 0;

  mytime = getLocalTime((time_t *)NULL);
  year = mytime->tm_year;
#ifndef WIN32
  free(mytime);
#endif
  return year;
}

int getMonth()
{
  struct tm *mytime;
  int month = 0;

  mytime = getLocalTime((time_t *)NULL);
  month = mytime->tm_mon;
#ifndef WIN32
  free(mytime);
#endif
  return month;
}
