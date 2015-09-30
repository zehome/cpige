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

#ifndef _H_DEBUG
#define _H_DEBUG

#include <stdio.h>

#define DEBUGLEVEL          3

#define DEBUG_ERR           1
#define DEBUG_MSG           2
#define DEBUG_ALL           3

#define DEBUG(priority, ...) _DEBUG(__LINE__, __FILE__, priority, __VA_ARGS__)

#define _ERROR(...)   DEBUG(DEBUG_ERR, __VA_ARGS__)
#define MESSAGE(...) DEBUG(DEBUG_MSG, __VA_ARGS__)
#define VERBOSE(...) DEBUG(DEBUG_ALL, __VA_ARGS__)

void _DEBUG(int _debug_line, char *_debug_filename, 
            int _debug_priority,  const char *_debug_message, ...);

void SetDebugFile(FILE *file);

#endif
