#ifndef __POOL_H
#define __POOL_H

#include "tool.h"

typedef struct urlPool_t
{
  serverSettings_t *settings;
  char *url;
  struct urlPool_t *next;
} urlPool_t;

urlPool_t *addPoolUrl(urlPool_t *pool, char *url);
urlPool_t *_getPool_pos(urlPool_t *pool, int pos);
serverSettings_t *getSettings(urlPool_t *pool, int *position);
int getPoolLen(urlPool_t *pool);

#endif
