#include "pool.h"
#include "tool.h"
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

urlPool_t *addPoolUrl(urlPool_t *pool, char *url)
{
  urlPool_t *new;
  
  if (pool != NULL)
  {
    /* Go to the end of the list */
    while (pool->next != NULL)
      pool = pool->next;
  }

  /* Allocate a new address */
  new = malloc(sizeof(urlPool_t));
  if (new == NULL)
  {
    perror("malloc");
    return pool;
  }

  new->url = strdup(url);

  new->settings = parseURL(url);
  new->next = NULL;

  if (new->settings == NULL)
  {
    _ERROR("Error parsing URL `%s'.\n", url);
    (void)free(new->url);
    (void)free(new);
    return pool;
  }
  
  if (pool != NULL)
    pool->next = new;
  else
    pool = new;

  MESSAGE("%s was added to pool.\n", url);
  
  return pool;
}

int getPoolLen(urlPool_t *pool)
{
  int i = 0;

  while (pool->next != NULL)
  {
    pool = pool->next;
    i++;
  }

  return i;
}

urlPool_t *_getPool_pos(urlPool_t *pool, int pos)
{
  int i;

  for (i = 0; ((i < pos) && (pool != NULL)); i++)
    pool = pool->next;

  return pool;
}

serverSettings_t *getSettings(urlPool_t *pool, int *position)
{
  serverSettings_t *set;
  urlPool_t *work;
  
  work = _getPool_pos(pool, *position);
  
  if ((*position+1) > (getPoolLen(pool)))
    *position = 0;
  else
    *position = *position+1;
  
  if (work == NULL)
    set = NULL;
  else
    set = work->settings;
    
  return set;
}
