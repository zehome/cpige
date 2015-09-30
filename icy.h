#ifndef __ICY_H
#define __ICY_H

enum
{
  SERVERTYPE_SHOUTCAST,
  SERVERTYPE_ICECAST
};

/* Icy headers */
typedef struct icyHeaders
{
  char *name;
  char *genre;
  char *notice1;
  char *notice2;
  char *content_type;
  char *url;
  /* type: 
   * 0 Shoutcast 
   * 1 Icecast 
   */
  int type; 
  int br;
  int metaint;
  int pub;
} icyHeaders;

icyHeaders *readicyheaders(char *headers);
void free_icy_headers(icyHeaders *icy);
void print_icyheaders(icyHeaders *_icy_headers );

#endif
