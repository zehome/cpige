#define VERSION "1.5"

#define IVAL_HOUR 1
#define IVAL_MIN  2

#include "pool.h"
#include "mynet.h"
#include "icy.h"

/* Currently played song */
typedef struct currentSong
{
  char *title;
  char *artist;
} currentSong;

/* Command line arguments are stored here */
typedef struct commandLine
{
  char *dir;
  char *logFile;
  char *pidFile;
  char *Next;
  int pige;
  int quiet;
  int live;
  int background;
  int usePigeMeta;
  int intervalType;
  int useNumbers;
  int weekBackup;
  int interval;
  int skipSongs;
  int useGUI;
  long long timeToStop;
} commandLine;
  
typedef struct cPigeUptime
{
  unsigned int day;
  unsigned int hour;
  unsigned int min;
  unsigned int sec;
} cPigeUptime;

typedef struct lastCut
{
  int hour;
  int min;
  int sec;
} lastCut;

/* explicit ? */
void print_credits ();

/* fetch song title from metadata */
char *getTitle(char *titleString);

/* parse metadata */
char *readMeta(int serversocket);

/* return a string pointer to print informations about current stream,
 * like title, bytes received, ... 
 */
char *statusLine(long long unsigned int uptime, long long unsigned int metacounter, int metaint, char *titre, char *nexttitre);

/* store informations about title currently played (for id3v1) */
currentSong *getCurrentSong(char *titre);

/* Parse command line arguments */
commandLine *parseCommandLine(int argc, char **argv);
void testCommandLine();

/* print help ... */
void print_help();

/* getUptime */
cPigeUptime *getUptime(long long unsigned int uptime);
char *getStats(long long unsigned int uptime, long long unsigned int metacounter, int metaint);

/* Intervals */
int checkInterval();
int mustCut(lastCut *cut);
int getCloserInterval(int now, int interval);
void checkWeekBackup();

#ifndef WIN32
  int getSongs(char *dir);
#endif

#define IS_PRINTABLE(c) (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) || (c == '-') || (c == ' ') || (c == '@'))

