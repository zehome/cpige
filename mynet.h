#ifndef __CPIGE_MYNET_H
#define __CPIGE_MYNET_H

#include "pool.h"
#include "icy.h"

#if WIN32
 #include <winsock2.h>
 #define sleep(msec) _sleep(msec)
#else
 #include <netdb.h>
 #include <netinet/in.h>
 #include <sys/socket.h>
 #include <arpa/inet.h>
#endif

#define SOCKET_TIMEOUT 10 /* Socket receives timeout after SOCKET_TIMEOUT seconds */
/* Please change this only if really needed... */
#define USER_AGENT "cPige/1.5"

extern int server_socket;
extern fd_set rfds;
extern struct timeval timeout;
extern urlPool_t *serversPool;
extern int poolPosition;

typedef struct stats
{
  unsigned long songs;
  unsigned long reconnections;
} stats;
extern stats *cPigeStats;

int server_connect (char *servername, int serverport);
int server_close (int serversocket);
int reconnect(int time, int _nb_tentatives, int _get_headers, icyHeaders *out_icy);
int sendHeaders(int serversocket, char *mountpoint, int metadata);
char *getHeaders(int serversocket);
int getHTTPCode(char *buffer);

#endif
