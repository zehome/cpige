/*******************************************\
| * Ce programme est sous licence GNU GPL  * |
| * This software is under GNU/GPL licence  * |
| * * * * * * * * * * * * * * * * * * * * * * |
| * http://www.gnu.org/copyleft/gpl.html    * |
 \*******************************************/

/* Par Laurent Coustet <ed@zehome.com>
 * http://ed.zehome.com/                    
 * Made by Laurent Coustet <ed@zehome.com>
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "time.h"

FILE *_Debug_Output = NULL;

void _DEBUG(int _debug_line, char *_debug_filename, 
            int _debug_priority,  const char *_debug_message, ...)
{
  /* arguments variables */
  char *z_format; /* thx dave */
  va_list ap;
  time_t now;
  struct tm *curTime = NULL;
      
  if (! _Debug_Output) return;
  
  now = time(NULL);
  if (now == (time_t)-1)
  {
    fprintf(stderr, "Can't log line: time() failed.\n");
    perror("time");
    return;
  }

#ifndef WIN32
  curTime = (struct tm *) malloc(sizeof(struct tm));
  localtime_r(&now, curTime); /* Get the current time */
#else
  curTime = localtime(&now);
#endif
  if (curTime == NULL)
  {
    fprintf(stderr, "Can't log line: localtime(_r)() failed.\n");
    return;
  }
          
  va_start(ap, _debug_message);  

  /* message de prioritée inférieure a notre prio, on vire */
  if (_debug_priority > DEBUGLEVEL)
    return;
    
  z_format = calloc(1024, 1);
  snprintf(z_format, 1023, "[%.2d:%.2d:%.2d] [DEBUG PRIO %d][File: %s][Line: %d] %s", curTime->tm_hour, curTime->tm_min, curTime->tm_sec, _debug_priority, _debug_filename, _debug_line, _debug_message);

  vfprintf(_Debug_Output, z_format, ap);
  
  fflush(_Debug_Output);

#ifndef WIN32
  (void)free(curTime);
#endif
  (void)free(z_format);
  va_end(ap);
  return;
}

void SetDebugFile(FILE *file)
{
  _Debug_Output = file;
  return;
}
