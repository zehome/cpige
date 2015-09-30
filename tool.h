#ifndef __TOOL_CPIGE_H
#define __TOOL_CPIGE_H

typedef struct serverSettings_t
{
  unsigned int port;
  char *serverAddr;
  char *mountpoint;
} serverSettings_t;

void *memory_allocation(int size);

serverSettings_t *parseURL(const char *url);
void printServSettings(serverSettings_t *set);
int stripcolon(const char *from, char *to);
void stripBadChar(const char *from, char *to);

#endif
