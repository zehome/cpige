#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mynet.h"
#include "debug.h"

/* Cette fonction se reconnecte au server servername,
 * port serverport, toute les time secondes, avec un
 * maximum de _nb_tentatives.
 *
 * Si _nb_tentatives est NULL, ou égal a -1, alors il n'y 
 * a aucune limite.
 *
 * Retourne la chausette créée
 */
int reconnect(int time, int _nb_tentatives, int _get_headers, icyHeaders *out_icy)
{
  int tentatives = 0;
  int server_socket;
  serverSettings_t *settings;
  icyHeaders *icy;
  
  if (!time) time = 2;
  
  if (serversPool == NULL)
  {
    _ERROR("Error: not any server defined.\n");
    exit(1);
  }
  
  /* Get an entry in the pool */
  settings = getSettings(serversPool, &poolPosition);
  if (settings == NULL)
  {
    _ERROR("No valid settings in urlPool.\n");
    exit(1);
  }

RECO:
  sleep(time);

  while ( (server_socket = server_connect(settings->serverAddr, settings->port)) < 0)
  {
    /* Get an entry in the pool */
    settings = getSettings(serversPool, &poolPosition);
    if (settings == NULL)
    {
      _ERROR("No valid settings in urlPool.\n");
      exit(1);
    }

    tentatives++;
    if ((tentatives > _nb_tentatives) && (_nb_tentatives != -1))
    {
      MESSAGE("Too many tentatives. Exiting program.\n");
      exit(-2);
    }
    MESSAGE("Reconnecting to http://%s:%d%s [try %d] in %d sec.\n", settings->serverAddr, settings->port, settings->mountpoint, tentatives, time);
    sleep(time);
  }

  VERBOSE("Time spent to reconnect: %d seconds, %d tentatives.\n", (tentatives * time), tentatives);

  if (sendHeaders(server_socket, settings->mountpoint, 1) <= 0)
  {
    _ERROR("Error sending headers: 0 byte sent.\n");
    goto RECO;
  }
  
  if (_get_headers)
  {
    if ( (icy = readicyheaders(getHeaders(server_socket))) == NULL)
      goto RECO;
    else
      out_icy = icy;
  }
  
  cPigeStats->reconnections++;
  
  return server_socket;
}

int server_connect (char *servername, int serverport)
{
  struct addrinfo hints;
  struct addrinfo *ai;
  char port[16];
  int res;

#if WIN32
  VERBOSE("Using win32 sockets\n");
  WSADATA WSAData;
  if((res = WSAStartup(MAKEWORD(2,0), &WSAData)) != 0)
    printf("Impossible d'initialiser l'API Winsock 2.0\n");
#endif
  VERBOSE("Entring Server_connect\n");
  /* on initialise la socket */
  VERBOSE("Servername: %s\n", servername);
  VERBOSE("Port: %d\n", serverport);
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  snprintf(port, sizeof(port), "%d", serverport);
  if ((res = getaddrinfo(servername, port, &hints, &ai)) != 0)
  {
    _ERROR("Error with getaddrinfo: %s\n",
        gai_strerror(res));
    return -1;
  }
  /* creation de la socket */
  if ( (server_socket = socket(ai->ai_family, ai->ai_socktype,
    ai->ai_protocol)) < 0)
  {
    _ERROR("Error creating shoutcast socket. Exiting.\n");
    return -2;
  }
  
  VERBOSE("Socket Creation Sucessful.\n");
  VERBOSE("Connection in progress...\n");

  /* requete de connexion */
  if(connect(server_socket, ai->ai_addr, ai->ai_addrlen) < 0 )
  {
    perror("socket");
    _ERROR("Remote host connection failed.\n");
    return -3;
  } else {
    VERBOSE("Connected.\n");
  } 
  
  FD_ZERO(&rfds);
  FD_SET(server_socket, &rfds);
  
  return server_socket;
}

int server_close (int serversocket)
{
  VERBOSE("Closing server connection.\n");
  shutdown(server_socket, 2);
  close(server_socket);
  server_socket = 0;
  VERBOSE("Server connection closed.\n");
  return -1;
}

int sendHeaders(int serversocket, char *mountpoint, int metadata)
{
  int ret = 0;
  char headers[256 + sizeof(USER_AGENT) + 16 + 11 + 4];
  
  if (mountpoint == NULL)
    snprintf(headers, 255, "GET / HTTP/1.0\r\n");
  else
    snprintf(headers, 255, "GET %s HTTP/1.0\r\n", mountpoint);

  if (metadata != 0)
    strncat(headers, "Icy-MetaData:1\r\n", 16);
  else
    strncat(headers, "Icy-MetaData:0\r\n", 16);

  strncat(headers, "User-Agent:", 11);
  strncat(headers, USER_AGENT, sizeof(USER_AGENT));
  strncat(headers, "\r\n\r\n", 4);
  
  ret = send(serversocket, headers, strlen(headers), 0);
  return ret;
}

char *getHeaders(int serversocket)
{
  int count = 0;
  int j, i = 0;
  int retval, errorCode = 0;
  char *ptr;
  char headers[4096]; /* Must check for overflow */
  char buffer[512];
  char c;

  memset(headers, 0, 4096);
  memset(buffer, 0, 512);

  /* For select() it's a global struct. */
  timeout.tv_sec = SOCKET_TIMEOUT;
  timeout.tv_usec = 0;

  retval = select(server_socket+1, &rfds, NULL, NULL, &timeout);
  if (retval <= 0)
  {
    _ERROR("Erreur de connexion in getHeaders().\n");
    goto error;
  }
  
  i = 0;
  
  while ( (recv(serversocket, &c, 1, 0) > 0) && (i < 512) )
  {
    if (c == '\r')
      continue;
    
    if (c == '\n')
      break;

    buffer[i++] = c;
  }

  errorCode = getHTTPCode(buffer);
  switch(errorCode)
  {
    case 200:
    case 301:
    case 302:
      break;
    default:
      _ERROR("Unknown error code received: %d\n", errorCode);
      goto error;
      break;
  }
  
  headers[0] = 0;

  for(j = 0, i = 0; ((j < 4096) && (count != 4)); j++)
  {
    /* For select() it's a global struct. */
    timeout.tv_sec = SOCKET_TIMEOUT;
    timeout.tv_usec = 0;
    retval = select(server_socket+1, &rfds, NULL, NULL, &timeout);
    if (retval <= 0)
    {
      _ERROR("Connection error in select (getHeaders).\n");
      goto error;

    } else if (recv(server_socket, &c, 1, 0) != 1) {
      _ERROR("Error reading data in getHeaders()\n");
      goto error;
    }
    
    if ((c == '\n') || (c == '\r'))
    {
       headers[i++] = c;
       count++;
    } else  {
       headers[i++] = c;
       count = 0;
    }
  }
  headers[i] = 0;
  
  if (!strlen(headers)) 
    return NULL;
  
  ptr = strdup(headers);
  printf("\n\n%s\n\n", ptr);

  return ptr;
  
error:
  server_close(server_socket);
  
  return NULL;
}

int getHTTPCode(char *buffer)
{
  char c;
  char array[256]; /* Must check for overflow */
  int i;
  int j = 0;
  int flag = 0;
  int returnCode = 0;

  if ((buffer == NULL) || (strlen(buffer) > 256)) return 0;

  for (i = 0; i < strlen(buffer) && j < 256; i++)
  {
    c = buffer[i];

    if (c == ' ')
    {
      if (flag)
      {
        array[j+1] = 0;
        flag = 0;
      } else
        flag = 1;
    }
    
    if (flag)
      array[j++] = c;

  } /* for */

  array[j] = 0;
  returnCode = atoi((char *) &array);

  return returnCode;
}


