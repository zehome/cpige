#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "icy.h"
#include "tool.h"
#include "debug.h"

icyHeaders *readicyheaders(char *headers)
{
  icyHeaders *icy;
  int i, j, k;
  char c;
  char line[50][1024]; /* une ligne ne peut dépasser 1024 octets. */
  int linecount;
  char prefix[512];
  char suffix[512];
  int state = 0;
  int headerslen = 0;
  
  prefix[0] = 0;
  suffix[0] = 0;

  if (headers == NULL) 
    return NULL; 
  else 
    headerslen = strlen(headers);
  
  icy = (icyHeaders *)memory_allocation(sizeof(icyHeaders));
  
  /* Usefull when radio doesn't specify icy name */
  
  icy->name         = NULL;
  icy->content_type = NULL;
  icy->url          = NULL;
  icy->notice1      = NULL;
  icy->notice2      = NULL;
  icy->genre        = NULL;
  icy->metaint      = 0;
  
  for(i = 0, j = 0, linecount = 0; i < headerslen; i++)
  {
    if (linecount >= 50) break;
    if (j >= 1023) 
    {
      line[linecount][j] = 0;
      linecount++;
      j = 0;
    }

    c = headers[i];

    if ((c == '\r') && (j == 0))
      break;
    
    if (c == '\n')
    {
      line[linecount][j] = '\n';
      line[linecount][j+1] = '\0';
      linecount++;
      j = 0;
    }
    else if (c != '\r')
      line[linecount][j++] = c;
  }

  for (i = 0; i < linecount; i++)
  {
    prefix[0] = 0;
    suffix[0] = 0;
    k = 0;
    state = 0;
    
    for (j = 0; j < strlen(line[i]); j++)
    {
      c = line[i][j];
      
      if (state == 0)
      {
        if (c == ':')
        {
          prefix[k] = '\0';
          k = 0;
          state = 1;
        } else
          prefix[k++] = c;
      
      } else if (state == 1) { /* On a rencontré les ":" */
        if (c != '\n')
        {
          if ((c == ' ') && (k == 0))
            continue;
          suffix[k++] = c;
        } else
          suffix[k] = '\0';
      }
    } /* for each linechar */
    
    if ((prefix[0] != '\0') && (suffix[0] != '\0'))
    {
      if (strncmp(prefix, "icy-notice1", 11) == 0)
        icy->notice1 = strdup(suffix);

      else if (strncmp(prefix, "icy-notice2", 11) == 0)
      {
        if (strstr("SHOUTcast", suffix) != NULL)
          icy->type = SERVERTYPE_SHOUTCAST;

        icy->notice2 = strdup(suffix);
      }

      else if (strncmp(prefix, "icy-name", 8) == 0)
        icy->name = strdup(suffix);

      else if (strncmp(prefix, "icy-genre", 9) == 0)
        icy->genre = strdup(suffix);

      else if (strncmp(prefix, "icy-url", 7) == 0)
        icy->url = strdup(suffix);

      else if (strncmp(prefix, "icy-pub", 7) == 0)
        icy->pub = (int) atoi((char *) &suffix);

      else if (strncmp(prefix, "icy-metaint", 11) == 0)
        icy->metaint = (int) atoi((char*) &suffix);

      else if (strncmp(prefix, "icy-br", 6) == 0 )
        icy->br = (int) atoi((char *) &suffix[0]);

      else if ((strncmp(prefix, "content-type", 12) == 0 ) ||
        (strncmp(prefix, "Content-Type", 12) == 0))
        icy->content_type = strdup(suffix);

      else if (strncmp(prefix, "Server", 6) == 0)
      {
        if (strstr(suffix, "Icecast") != NULL)
          icy->type = SERVERTYPE_ICECAST;
      }
    }
  } /*  for eachline */
  
  
  if (icy->name == NULL)
    icy->name = strdup("No Name");

  if (icy->content_type == NULL)
    icy->content_type = strdup("audio/mpeg");

  if (icy->metaint != 0)
    VERBOSE("Using metaint: %d\n", icy->metaint);
  else {
    VERBOSE("Using default metaint: %d\n", 32 * 1024);
    icy->metaint = 32*1024;
  }

  free(headers);
  return icy;
}

void free_icy_headers(icyHeaders *icy)
{
  if (! icy) return;
  if (icy->name)         free(icy->name);
  if (icy->genre)        free(icy->genre);
  if (icy->notice1)      free(icy->notice1);
  if (icy->notice2)      free(icy->notice2);
  if (icy->url)          free(icy->url);
  if (icy->content_type) free(icy->content_type);
}

void print_icyheaders(icyHeaders *_icy_headers )
{
  if (_icy_headers == NULL)
    return;
  
  printf("metaint: %d\n",       _icy_headers->metaint);
  printf("bitrate: %d\n",       _icy_headers->br);
  printf("icy-notice1: %s\n",   _icy_headers->notice1);
  printf("icy-notice2: %s\n",   _icy_headers->notice2);
  printf("icy-name: %s\n",      _icy_headers->name);
  printf("icy-genre: %s\n",     _icy_headers->genre);
  printf("icy-url: %s\n",       _icy_headers->url);
  printf("content_type: %s\n",  _icy_headers->content_type);
}
