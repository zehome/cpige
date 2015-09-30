#include "tool.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

serverSettings_t *parseURL(const char *url)
{
  serverSettings_t *settings;
  int i, j, k;
  short flag = 0;
  char c;
  char *tmpPort;

  if (url == NULL)
    return NULL;
  
  settings = (serverSettings_t *) malloc(sizeof(serverSettings_t));
  if (settings == NULL)
  {
    perror("malloc");
    return NULL;
  }
  
  tmpPort = (char *) memory_allocation(5 + 1); /* Maxport == 65535 + 1 (\0) */
  
  if (strstr(url, "://") == NULL)
    return NULL;
  
  /* Init */
  settings->port = 80;
  settings->mountpoint = "/";
  settings->serverAddr = NULL;
  for (i = 0, j = 0, k = 0; i <= strlen(url); i++)
  {
    c = url[i];

    /* Flag == 0: on cherche la fin de http://
     */
    if ((flag == 0) && (c == '/') && ((i+1) <= strlen(url)))
    {
      if (url[(i+1)] == '/')
      {
        i++;
        settings->serverAddr = (char *) memory_allocation(strlen(url) - 5);
        flag = 1;
      }
    /* Flag == 1: 
     * on a un proto de type "kjklsjdf://", on remplie directement host
     */
    } else if (flag == 1) {
      if (c == ':')
      {
        settings->serverAddr[j] = 0;
        j = 0;
        flag = 2;
      } else if (c == '/') {
        settings->serverAddr[j] = 0;
        settings->port = 80;
        j = 0;
        flag = 3;
        settings->mountpoint = (char *) memory_allocation(strlen(url) +1 );
        settings->mountpoint[j++] = '/';
      } else {
        settings->serverAddr[j++] = c;
      }
    /* Flag == 2:
     * On a trouvé un :, on a donc le port en suite
     */
    } else if (flag == 2) {
      /* port de comm */
      if (c != '/')
      {
        if (j < 5)
          tmpPort[j++] = c;
      } else {
        if (j <= 5)
          tmpPort[j] = 0;
        j = 0;
        settings->port = atoi(tmpPort);
        flag = 3;
        settings->mountpoint = (char *) memory_allocation(strlen(url) - strlen(tmpPort) + 1);
        /* We should remove 7 bytes for http:// but it'll not work if http:// not given,
         * so I'm not removing 7 bytes.
         */
        settings->mountpoint[j++] = '/';
        (void) free(tmpPort);
      }
    /* Flag == 3
     * on cherche le mountpoint 
     */
    } else if (flag == 3) {
      if (i >= strlen(url)) {
        settings->mountpoint[j] = 0;
        break;
      } else {
        settings->mountpoint[j++] = c;
      }
    }
  } /* For */

  /*  printServSettings(settings); */
  return settings;
}

void printServSettings(serverSettings_t *set)
{
  printf("host: %s\n", set->serverAddr);
  printf("port: %d\n", set->port);
  printf("mp: %s\n", set->mountpoint);
  return;
}

/* Usefull for debugging :p */
void *memory_allocation(int size)
{
  void *ptr;
  
  if ((ptr = calloc(size, sizeof(char))) == NULL) {
    perror("Error allocating memory. Out Of Memory ?\n");
    exit(-1);
  }
  
  return ptr;
}

int stripcolon(const char *from, char *to)
{
  int len = 0;
  
  if (! from)
    return 0;

  while (*from != '\0')
  {
    switch (*from)
    {
      case ':':
        break;
      default:
        len++;
        *to++ = *from;
        break;
    }
    
    from++;
  }

  return len;
}

void stripBadChar(const char *from, char *to)
{
  if (from == NULL)
    return;

  while (*from != '\0')
  {
    switch (*from)
    {
      case '|':
        *to++ = 'l';
        break;
      case '`':
        *to++ = '\'';
        break;
      case '/':
      case '\\':
        break;
        
      default:
        if (isascii(*from))
          *to++ = *from;
        break;
    }
    from++;
  }
  
  *to = '\0';
}
