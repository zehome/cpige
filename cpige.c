/* By Laurent Coustet (c) 2005 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifndef WIN32
  #include <arpa/inet.h>
#endif

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>

#ifndef WIN32
 #include <regex.h>
 #include <locale.h>
#endif

#include "cpige.h"
#include "icy.h"
#include "id3.h"
#include "pool.h"
#include "tool.h"
#include "mytime.h"
#include "debug.h"

#ifndef NOCONFIG
 #include "configlib.h"
 char *configFile = NULL;
#endif

/* Windows needs file to be opened in byte mode */
#if WIN32
 #define WAITALL 0
 #define WRITE "wb+"
 #define sleep(msec) _sleep(msec)
#else
 #define WAITALL 0x100
 #define WRITE "w+"
#endif

#define TENTATIVES -1
#define CONNECTION_TIMEOUT 5 /* The connection must be established within 5 seconds */
#define RECONNECT_TIME 5 /* We are waiting 5 seconds before trying to reconnect. */

/* Struct storing command line arguments */
commandLine *cmdLine;

/* Uptime */
time_t start_time;
time_t current_time;
/* for select() */
fd_set rfds;
/* socket timeout */
struct timeval timeout;

/* Statistics */
stats *cPigeStats;

/* Servers pool */
urlPool_t *serversPool = NULL;
int poolPosition = 0;

int server_socket = -1;

#ifdef SOLARIS
int daemon(int nochdir, int noclose);
#endif

/* Main Program */
int main (int argc, char **argv)
{
  char *buffer      = NULL;
  char *titre       = NULL;
  char *oldtitre    = NULL;
  char *nexttitre   = NULL;
  char *filename    = NULL;
  char *extension   = "mp3";
  char *meta;
  char *statusline;
  char *dir;

  long long unsigned int uptime      = 0;
  long long unsigned int metacounter = 0;
 
  /* When operates in quiet mode, display stats in logfile
   * evry countdown seconds
   */
  time_t countdown = 3600; /* Evry 1 hour */
  time_t temptime;

  /* Integers ... */
  int size;
  int retval;
  int i;
  int tempsize = 0;
  int nextsize = 0;
  int len1     = 0;
  int len2     = 0;
  int maxlen   = 0;
  int songs    = 0;
  int buflen   = 0;
  int maxlinelength = 80;

  /* Directory & File manipulations */
  DIR *pige_dir;
  FILE *output_file = NULL;
  FILE *logfile     = NULL;

  /* Custom typedef */
  currentSong *curSong;
  icyHeaders *icy_headers = NULL;
  lastCut last_cut;

  /*
   * last_cut.hour = getHour();
   * last_cut.min  = getMinute();
   */

  last_cut.hour = -1;
  last_cut.min =  -1;
  last_cut.sec =  -1;
  
  /* Global dynamic configuration */
  cmdLine = parseCommandLine(argc, argv);
  testCommandLine();

  /* Logfile */
  if ((logfile = fopen(cmdLine->logFile, "a+")) == NULL)
  {
    _ERROR("Unable to openlogfile: %s Setting log to stdout\n", cmdLine->logFile);
    logfile = stdout;
  } else {
    VERBOSE("Successfully opened %s\n", cmdLine->logFile);
  }
  
  /* We setup debug to logfile instead of stderr */
  SetDebugFile(logfile);
  
  if (cmdLine->background)
  {
#ifndef WIN32
    int pid;
    int fd;
    FILE *pidfile;

    if (daemon(1, 1) == -1)
    {
      _ERROR("Error daemonizing. %s\n", strerror(errno));
      _exit(-1);
    }
    
    pid = (int)getpid();
    
    if (cmdLine->pidFile != NULL)
    {
      pidfile = fopen(cmdLine->pidFile, "w");
      if (pidfile == NULL)
        _ERROR("Unable to open pidfile %s\n", cmdLine->pidFile);
      else
      {
        fprintf(pidfile, "%d", pid);
        fclose(pidfile);
      }
    }
    
    fprintf(stdout, "cPige launched in background. pid: %d\n", pid);
    fd = open("/dev/null", O_RDWR);
    
    if (fd == -1)
    {
      _ERROR("Error opening /dev/null: %s\n", strerror(errno));
      _exit(0);
    }
    
    for (i = 0; i < 3; i++)
      dup2(fd, i);
    
    close(fd);
#else
   printf("Fork not available under WIN32.\n");
#endif
  }

  cPigeStats = (stats *)memory_allocation(sizeof(stats));
  cPigeStats->reconnections = 0;
  cPigeStats->songs = 0;
  
  /* Create output dir if does not exists! */
  if (( pige_dir = opendir(cmdLine->dir)) == NULL)
  {
    _ERROR("Unable to open %s for writing\n", cmdLine->dir);
#ifdef WIN32
    if (mkdir(cmdLine->dir) != 0) {
#else
    if (mkdir(cmdLine->dir, 0755) != 0) {
#endif
      _ERROR("Failed trying to create %s. Verify rights.\n", cmdLine->dir);      
      _exit(-1);
    } else {
      VERBOSE("Created %s\n", cmdLine->dir);
    }
  } else {
    VERBOSE("Sucessfully opened %s\n", cmdLine->dir);
    closedir(pige_dir);
  }

  /* If we are keeping One week of stream, check if directories
   * exists, and check disk space
   */
  
  checkWeekBackup();

  /* Start time, for uptime calculation */
  start_time = time((time_t *)NULL);
  temptime = time((time_t *)NULL) + countdown;

  /* We try connecting and receiving good headers infinitely */
  do
  {
    server_socket = reconnect(RECONNECT_TIME, TENTATIVES, 0, NULL);
    if (icy_headers != NULL)
    {
      free_icy_headers(icy_headers);
      free(icy_headers);
      icy_headers = NULL;
    }

    icy_headers = readicyheaders(getHeaders(server_socket));
  } while (icy_headers == NULL);
  
  if (icy_headers->metaint != buflen)
  {
    buflen = icy_headers->metaint;
    if (buffer != NULL)
      free(buffer);
    buffer = (char *)memory_allocation(buflen);
  }
  
  if (strncmp(icy_headers->content_type, "audio/mpeg", 10) == 0)
    extension = "mp3";
  else if (strncmp(icy_headers->content_type, "audio/aac", 9) == 0)
    extension = "aac";
  else if (strncmp(icy_headers->content_type, "application/ogg", 15) == 0)
    extension = "ogg";

  VERBOSE("Using extension: %s\n", extension);

  /* Only usefull for debug, not for production ! */
  /* print_icyheaders(icy_headers); */

#ifndef WIN32
  if(cmdLine->useNumbers == 1)
    songs = getSongs(cmdLine->dir);
#endif

  nextsize = 512;
  oldtitre = strdup("please.delete");
  
  while (1)
  {
    /* For select() it's a global struct. */
    timeout.tv_sec = SOCKET_TIMEOUT;
    timeout.tv_usec = 0;
    retval = select(server_socket+1, &rfds, NULL, NULL, &timeout);
    if (retval <= 0) 
    {
      _ERROR("Connection Error.\n");
      server_close(server_socket);
      server_socket = reconnect(RECONNECT_TIME, TENTATIVES, 1, icy_headers);
      if (icy_headers->metaint != buflen)
      {
        buflen = icy_headers->metaint;
        free(buffer);
        buffer = (char *)memory_allocation(buflen);
      }

      tempsize = 0;
      nextsize = 512;
      continue;
    }

    size = recv(server_socket, buffer, nextsize, 0);
    if ((size == -1) || ((size == 0) && nextsize != 0))
    {
      _ERROR("Connection error in recv main() size: %d nextsize: %d\n", size, nextsize);
      server_socket = reconnect(RECONNECT_TIME, TENTATIVES, 1, icy_headers);
      if (icy_headers->metaint != buflen)
      {
        buflen = icy_headers->metaint;
        free(buffer);
        buffer = (char *)memory_allocation(buflen);
      }

      tempsize = 0;
      nextsize = 512;
      continue;
    }
    
    if (output_file)
    {
      if (fwrite(buffer, sizeof(char), size, output_file) != size*sizeof(char))
      {
        _ERROR("Error writing output. Disk full ?\n");
        break;
      }
      /* Many thanks to the #hurdfr team! */
      fflush(output_file);
    }

    if ( tempsize == icy_headers->metaint )
    {
      if (cmdLine->pige)
      {
        /* Pige Mode */
        
        if (mustCut(&last_cut) == 1)
        {
          if ((cmdLine->usePigeMeta == 1) && (output_file != NULL) && (strncmp(icy_headers->content_type, "audio/mpeg", 10) == 0))
          {
            char buffer1[30];
            char buffer2[30];
            char *buffer3;
            
            switch (cmdLine->intervalType)
            {
              case IVAL_HOUR:
                snprintf(buffer1, 30, "pige %.2dh -> %.2dh", last_cut.hour, getCloserInterval(getHour(), cmdLine->interval));
                break;
              case IVAL_MIN:
                snprintf(buffer1, 30, "pige %.2dh%.2d -> %.2dh%.2d", last_cut.hour, last_cut.min, getCloserInterval(getHour(), 1), getCloserInterval(getMinute(), cmdLine->interval));
                break;
            }
            strncpy(buffer2, icy_headers->name, 30);
            
            buffer3 = GetId3v1(buffer2, buffer1, icy_headers->name);
            fwrite(buffer3, sizeof(char), 128, output_file);
            (void)free(buffer3);
          }
          
          if (output_file)
            fclose(output_file);

          if (cmdLine->weekBackup)
          {
            char *dirname = getDayName(time((time_t *)NULL));
            int dirlen = strlen(cmdLine->dir);
            int totlen = dirlen+strlen(dirname)+1+1+1;
            
            dir = (char *)memory_allocation(totlen);
            if (cmdLine->dir[dirlen-1] == '/')
              snprintf(dir, totlen, "%s%s/", cmdLine->dir, dirname);
            else
              snprintf(dir, totlen, "%s/%s/", cmdLine->dir, dirname);

            (void)free(dirname);
          } else {
            dir = strdup(cmdLine->dir);
          }
          
          switch (cmdLine->intervalType)
          {
            case IVAL_HOUR:
              last_cut.hour = getCloserInterval(getHour(), cmdLine->interval);
              last_cut.min  = getCloserInterval(getMinute(), 60);

              len1 = strlen(dir) + 1 + strlen(extension) + 2 + 1;
              filename = (char *)memory_allocation(len1);
              snprintf(filename, len1, "%s%.2d.%s", dir, last_cut.hour, extension);
              break;
              
            case IVAL_MIN:
              last_cut.min  = getCloserInterval(getMinute(), cmdLine->interval);
              last_cut.hour = getCloserInterval(getHour(), 1);
              
              len1 = strlen(dir) + 1 + strlen(extension) + 2 + 2 + 1 + 1;
              filename = memory_allocation(len1);
              snprintf(filename, len1, "%s%.2d-%.2d.%s", dir, getHour(), last_cut.min, extension);
              break;
              
            default:
              fprintf(stderr, "Internal Error: unknown interval type.\n");
              break;
          }
          last_cut.sec = time(NULL);

          (void)free(dir);

          if ((output_file = fopen(filename, "r")) != NULL)
          {
            VERBOSE("Erasing %s\n", filename);
            unlink(filename);
            fclose(output_file);
          }
          VERBOSE("Opening: %s\n", filename);
          output_file = fopen(filename, WRITE);
          (void)free(filename);

        }
      } else if (cmdLine->live) {
        printf("Not yet implemented.\n");
        _exit(1);
      }
      
      metacounter++;
      current_time = time((time_t *)NULL);
      uptime = (long long unsigned int) difftime(current_time, start_time);
      
      /* Classical stdout status line */
      if (!cmdLine->quiet)
      {
        fflush(stdout);
        fprintf(stdout, "\r");
      
        /* 100% pur porc */
        for(i = 0; i < maxlinelength; i++)
          fprintf(stdout, " ");
       
        if (strncmp(icy_headers->content_type, "application/ogg", 15) == 0)
          statusline = statusLine(uptime, metacounter, icy_headers->metaint, NULL, NULL);
        else
          statusline = statusLine(uptime, metacounter, icy_headers->metaint, oldtitre, nexttitre);

        fprintf(stdout, "\r%s", statusline);
      
        if ( strlen(statusline) > maxlinelength)
          maxlinelength = strlen(statusline);

        if (statusline != NULL) 
          (void)free(statusline);
      }

      if ((cmdLine->useGUI) && (cmdLine->quiet))
      {
        char *mybuf;

        /* GUI Communication protocol */
        /* Protocol:
         * CPIGEGUI:title:nexttitle:timeleft\n
         */
        fprintf(stdout, "CPIGEGUI:");
        
        if (oldtitre)
        {
          mybuf = (char *)memory_allocation(strlen(oldtitre)+1);
          stripcolon(oldtitre, mybuf);
          fprintf(stdout, "%s", mybuf);
          free(mybuf);
        }
        
        fprintf(stdout, ":");
        
        if (nexttitre)
        {
          mybuf = (char *)memory_allocation(strlen(nexttitre)+1);

          if ((cmdLine->Next) && (strlen(nexttitre) > strlen(cmdLine->Next)))
            stripcolon(nexttitre + strlen(cmdLine->Next), mybuf);
          else
            stripcolon(nexttitre, mybuf);

          fprintf(stdout, "%s", mybuf);
          free(mybuf);
        } else
          fprintf(stdout, " ");

        fprintf(stdout, ":");

        if (cmdLine->pige == 1)
        {
          int timeleft, percentage;
          int stop_s, nowTmp;
          int tmp1;
            
          nowTmp = time((time_t *)NULL);
          
          /* How many time left ? */
          switch (cmdLine->intervalType)
          {
            case IVAL_HOUR:
              /* Il me faut l'heure d'arrivée théorique, en seconde, du stream */
              tmp1 = (getCloserInterval(getHour(), cmdLine->interval) + cmdLine->interval);
              stop_s = ((tmp1 * 3600) + (nowTmp - ((getHour()*3600) - getMinute() * 60) - getSec()));
              percentage = ((float)(1.0 - (((float)stop_s - (float)nowTmp) / ((float)cmdLine->interval*3600.0))) * 100.0);
              break;
            case IVAL_MIN:
              tmp1 = (getCloserInterval(getMinute(), cmdLine->interval) + cmdLine->interval);
              stop_s = ((tmp1 * 60) + (nowTmp - (getMinute() * 60) - getSec()));
              percentage = ((float)(1.0 - (((float)stop_s - (float)nowTmp) / ((float)cmdLine->interval*60.0))) * 100.0);
              //percentage = ((float)(1.0 - (((float)tmp1 - (float)getMinute()) / (float)getMinute())) * 100.0);
              break;
          }

          timeleft = stop_s - nowTmp;

          fprintf(stdout, "%d:%d", timeleft, percentage);
        }
        
        fprintf(stdout, "\n");
        fflush(stdout);
      }

      /* Stats evry countdown seconds */
      if ((countdown != 0) && (temptime <= current_time))
      {
        statusline = getStats(uptime, metacounter, icy_headers->metaint);
        if (statusline != NULL)
        {
          if (fwrite(statusline, sizeof(char), strlen(statusline), logfile) != strlen(statusline))
          {
            _ERROR("Fwrite error.\n");
            break; 
          }
          /* Many thanks to the #hurdfr@freenode team! */
          fflush(logfile);
          (void)free(statusline);
        } else {
          VERBOSE("getStats returned NULL values...\n");
        }
        temptime = current_time + countdown;
      }
      
      if ((strncmp(icy_headers->content_type, "audio/mpeg", 10) == 0) ||
          (strncmp(icy_headers->content_type, "audio/aacp", 10) == 0))
      {
        meta = readMeta(server_socket);
      
        if ((meta == NULL) && (server_socket == 0))
        {
          server_socket = reconnect(RECONNECT_TIME, TENTATIVES, 0, icy_headers);
          if (icy_headers->metaint != buflen)
          {
            buflen = icy_headers->metaint;
            free(buffer);
            buffer = (char *)memory_allocation(buflen);
          }

          tempsize = 0;
          nextsize = 512;
          continue;
        }
        
        if (titre != NULL)
          free(titre);

        titre = getTitle(meta);

        if (meta != NULL)
          (void)free(meta);
      }

      if (titre != NULL)
      {
        if (strlen(titre) > 0)
        {
          if (strstr(titre, cmdLine->Next) == NULL) /* If title isn't ASUIVRE */
          {
            if (oldtitre != NULL)
            {
              len1 = strlen(oldtitre);
              len2 = strlen(titre);
             
              /* We are determining the maximal comp len */
              if (len1 >= len2); else maxlen = len1;
               
              if (strncmp(titre, oldtitre, maxlen) != 0)
              {
                if (! cmdLine->pige)
                {
                  if (cmdLine->skipSongs > 0)
                  {
                    cmdLine->skipSongs--;
                    VERBOSE("Skipping song `%s' (left %d songs to skip)\n", titre, cmdLine->skipSongs);
                  } else {
                    cPigeStats->songs++;
                    songs++;
                  
                    if (output_file != NULL)
                    {
                      curSong = getCurrentSong(oldtitre);
                      if (curSong != NULL)
                      {
                        if (strncmp(icy_headers->content_type, "audio/mpeg", 10) == 0)
                        {
                          char *buffer1;
                          buffer1 = GetId3v1(curSong->title, curSong->artist, icy_headers->name);
                          fwrite(buffer1, sizeof(char), 128, output_file);
                          (void)free(buffer1);
                        }
                        if (curSong->artist != NULL)
                          free(curSong->artist);
                        if (curSong->title != NULL)
                          free(curSong->title);
                        free(curSong);
                      }

                      fclose(output_file);
                    }
                    
                    if (cmdLine->useNumbers == 0)
                    {
                      len1 = strlen(titre) + strlen(cmdLine->dir)+ 1 + strlen(extension) + 1 + 1;
                      filename = (char *)memory_allocation(len1);
                      snprintf(filename, len1, "%s%s.%s", cmdLine->dir, titre, extension);
                    } else {
                      len1 = 5+strlen(titre) + strlen(cmdLine->dir)+ 1 + strlen(extension) + 1 + 1;
                      filename = (char *)memory_allocation(len1);
                      snprintf(filename, len1, "%s%.4d_%s.%s", cmdLine->dir, songs, titre, extension);
                    }

                    if ((output_file = fopen(filename, "r")) == NULL)
                    { /* Anti doublons */
                      VERBOSE("New file opened: %s\n", filename);
                      output_file = fopen(filename, WRITE);
                    } else {
                      VERBOSE("File already exists %s.\n", filename);
                      fclose(output_file);
                      output_file = NULL;
                    }
                    (void)free(filename);
                  } /* Song skip */
                }
              } /* Title are differents */
            } /* Oldtitre != NULL */
            if (oldtitre != NULL)
              (void)free(oldtitre);

            oldtitre = strdup(titre);

          } else { 
            /* Title is "ASUIVRE" */
            if (nexttitre != NULL) 
              (void)free(nexttitre);
            
            nexttitre = strdup(titre);
          }
          if (titre != NULL)
          {
            (void)free(titre);
            titre = NULL;
          }
          
        } /* Strlen(titre > 0) */
      }
      tempsize = 0; /* tempsize = 0: chunk received successfully. Awaiting next chunk */
    } else if (tempsize > icy_headers->metaint) {
      _ERROR("Error tempsize > metaint\n");
      break;
    } else
      tempsize = tempsize + size;

    if ((tempsize + 512) >= icy_headers->metaint)
      nextsize = icy_headers->metaint - tempsize;
    else
      nextsize = 512;

    /* Check timeToStop */
    if ((cmdLine->timeToStop != -1) && (uptime >= cmdLine->timeToStop))
    {
      VERBOSE("TimeToStop reached. Stopping cPige.\n");
      goto cleanup;
    }
  } /* infinite loop */

cleanup:
  printf("\n");
  fflush(stdout);

  server_close(server_socket);

  /* cleanup */
  if (icy_headers != NULL) 
    (void)free(icy_headers);
  if (output_file != NULL) 
    fclose(output_file);
  if (logfile != stdout) 
    fclose(logfile);
  
  (void)free(cmdLine);
  (void)free(cPigeStats);
  
  return 0;
}

cPigeUptime *getUptime(long long unsigned int uptime)
{
  cPigeUptime *cu;
  
  cu = (cPigeUptime *)memory_allocation(sizeof(cPigeUptime));

  /* Calculs bourrins :) */
  cu->day  = (unsigned int) (uptime / (60 * 60 * 24));
  cu->hour = (unsigned int) (uptime / (60 * 60)) - (cu->day * 24);
  cu->min  = (unsigned int) (uptime / (60)) - ((cu->hour * 60) + (cu->day * 24 * 60));
  cu->sec  = (unsigned int) (uptime) - ((cu->min * 60) + (cu->hour * 60 * 60) + (cu->day * 24 * 60 * 60));
  
  return cu; 
}

char *getStats(long long unsigned int uptime, long long unsigned int metacounter, int metaint)
{
  char *line;
  cPigeUptime *cu;
  
  cu = getUptime(uptime);
  line = (char *)memory_allocation(300); /* Exessif. */
  
  snprintf(line, 299, "Uptime: %d days, %d hours, %d min, %d seconds\nDownloaded: %lldKo\nSongs played: %ld\nReconnections: %ld\n", cu->day, cu->hour, cu->min, cu->sec, (long long unsigned int)((metacounter*metaint)/1024), cPigeStats->songs, cPigeStats->reconnections);

  free(cu);
  return line;
}

char *statusLine(long long unsigned int uptime, long long unsigned int metacounter, int metaint, char *titre, char *nexttitre)
{
  char *line;
  int len;
  cPigeUptime *cu;
  
  cu = getUptime(uptime);
  
  /* Pas terrible... */
  line = memory_allocation(300);
  
  len = snprintf(line, 299, "[%dj %dh:%dm:%ds][%lldKo] ", cu->day, cu->hour, cu->min, cu->sec, (long long unsigned int)((metacounter * metaint) / 1024));
  
  if (cmdLine->pige)
    len += snprintf(line + len, 299-len, "%dh -> %dh ", getHour(), getHour()+1);
  
  if (titre != NULL)
    len += snprintf(line+len, 299-len, "%s", titre);
  
  if (nexttitre != NULL) 
    if (strstr(nexttitre, titre) == NULL) 
      snprintf(line+len, 299-len, " -> %s", nexttitre);

  free(cu);
  return line;
}


char *readMeta(int serversocket)
{
  char *meta;
  char c;
  int retval;
  int size = 0;
  int readsize = 0;
  char *buffer;
  
  /* For select() it's a global struct. */
  timeout.tv_sec = SOCKET_TIMEOUT;
  timeout.tv_usec = 0;

  retval = select(server_socket+1, &rfds, NULL, NULL, &timeout);
  if (retval <= 0)
  {
    _ERROR("Erreur de connexion!\n");
    server_close(server_socket);
    return NULL;
  } else if (recv(server_socket, &c, 1, 0) != 1) {
    _ERROR("Error reading from server socket: \n", strerror(errno));
    server_close(server_socket);
    return NULL;
  }
  
  if (c > 0)
  {
    meta   = (char *)memory_allocation((c*16)+2);
    buffer = (char *)memory_allocation((c*16)+1);

    /* For select() it's a global struct. */
    timeout.tv_sec = SOCKET_TIMEOUT;
    timeout.tv_usec = 0;
    retval = select(server_socket+1, &rfds, NULL, NULL, &timeout);
    if (retval <= 0)
    {
      _ERROR("Connection error in select. (readmeta)\n");
      (void)free(buffer);
      (void)free(meta);
      server_close(server_socket);
      return NULL;

    } else {
      readsize = 0;
      while(readsize < c*16)
      {
        /* For select() it's a global struct. */
        timeout.tv_sec = SOCKET_TIMEOUT;
        timeout.tv_usec = 0;

        retval = select(server_socket+1, &rfds, NULL, NULL, &timeout);
        if (retval <= 0)
        {
          _ERROR("Erreur de connexion!\n");
          server_close(server_socket);
          return NULL;
        } else {
          size = recv(server_socket, buffer, (c*16 - readsize), 0);
          if (size <= 0)
          {
            VERBOSE("Megaproblem here.\n");
            server_close(server_socket);
          }
          readsize += size;
          strncat(meta, buffer, size);
        }
      }
      (void)free(buffer);
    }
  } else {
    /* Title was not sent. */
    return NULL;
  }
  return meta;
}


/* Lorsque le serveur envoie: "StreamTitle='TITREZIK';"
 * On ne récupère avec cette fonction que TITREZIK.
 */

char *getTitle(char *titleString)
{
  int i;
  char *result;
  int count = 0;
  int j = 0;
  int first = 0;
  
  if ((titleString == NULL) || (strlen(titleString) == 0))
    return NULL;
  
  if (strstr(titleString, "StreamTitle") == NULL)
  {
    /* We have there an invalid strea title. Can't be
     * parsed.
     *
     * Aborting, will try another server!
     */
    _ERROR("Received a stream with metadata enabled, but at metaint interval, we only received binary crap. Please fix your stream.");
    return NULL;
  }
  
  result = (char *)memory_allocation(strlen(titleString) + 1);
  
  for (i=0; i < strlen(titleString); i++)
  {
    if (count == 1)
    {
      if (titleString[(i+1)] != ';')
      {
        if (first == 0)
        {
          if ((isalnum((int) titleString[i])) && (titleString[i] != ' '))
          {
            result[j++] = titleString[i];
            first++;
          }
        } else {
          result[j++] = titleString[i];
        }
      }
    }
    
    if (titleString[i] == '\'')
    {
      if (count == 0)
        count++;
      else if (titleString[(i+1)] == ';')
        break;
    }
  } /* for */
  
  result[j] = '\0';
  
  stripBadChar(result, result);
  
  return result;
}

currentSong *getCurrentSong(char *title) 
{
  currentSong *cursong;
  char c;
  int i;
  int j = 0;
  int flag = 0;

  if (title == NULL) return NULL;

  cursong = (currentSong *)malloc(sizeof(currentSong));
  
  cursong->title  = (char *)memory_allocation(30 + 1); /* id3v1 == 30 chars max */
  cursong->artist = (char *)memory_allocation(30 + 1); /* id3v1 == 30 chars max */
  
  for (i = 0; i < strlen(title); i++)
  {
    c = title[i];
    if (!flag) /* ! Flag */
    {
      if ((c == '-') && ((j-1) > 0))/* Avant le - => artiste */
      {
        if (cursong->artist[(j-1)] == ' ')
          cursong->artist[(j-1)] = '\0';

        flag = 1;
        j = 0; /* Pour l'espace */
      } else {
        if (j < 30) /* "l'espace!" */
          cursong->artist[j++] = c;
      }
    } else { /* Flag */
      if (j < 30)
      {
        if (j == 0 && c == ' ') continue;
        cursong->title[j++] = c;
      }
    }
  } /* boucle :) */

  if (strlen(cursong->title) == 0)
  {
    (void)free(cursong->title);
    cursong->title = NULL;
  }
  
  if (strlen(cursong->artist) == 0)
  {
    (void)free(cursong->artist);
    cursong->artist = NULL;
  }

  /* Last byte of artist & title in the structure
   * are 0 as they have been initialized with calloc.
   */
  return cursong;
}

void print_credits()
{
  printf ("cPige %s by Laurent Coustet (c) 2005\n", VERSION);
  return;
}

void print_help()
{
  fprintf (stderr,
          "cPige help. cPige is a Laurent Coustet product.\n"
          "For more informations about me and my software,\n"
          "please visit http://ed.zehome.com/\n\n"
#ifdef NOCONFIG
          "Usage: ./cpige -h http://stream-hautdebit.frequence3.net:8000/ -d /home/ed/Pige -l logfile.log\n\n"
          "    -h host to connect to.\n"
          "    -V show cpige Version.\n"
          "    -d directory save stream to this directory.\n"
          "    -P Pige mode (takes no argument), save stream by hour.\n"
          "    -M Use pige Meta: will write id3v1 tag (only usefull with pige mode).\n"
          "    -q Quiet mode, does not output stream status.\n"
          "    -b Background mode (UNIX only) use cPige as a daemon.\n"
          "    -l Path to logfile.\n"
          "    -I [h|m] pige mode will cut on a hour by hour basis or min by min basis.\n"
          "    -i nb how many \"nb\" hour(s) or minute(s) we should wait before cutting.\n"
#ifndef WIN32
          "    -n cPige will append xxxx to file in 'non pige mode', where xxxx is a number.\n");
#else
          );
#endif
#else
          "Usage: ./cpige -c path/to/cpige.conf\n"
          "    -c path/to/cpige.conf.\n");
#endif
  fflush(stderr);
}

commandLine *parseCommandLine(int argc, char **argv)
{
  commandLine *cmdLine;
  int i;
  char *c;

#ifndef NOCONFIG
  char *buffer;
  config_t *conf, *workconf;
#endif

  if (argc < 2)
  {
    print_help();
    _exit(-1);
  }
  
  cmdLine = (commandLine *)malloc(sizeof(commandLine));
  
  /* default is not pige mode */
  cmdLine->pige  = 0;
  cmdLine->quiet = 0;
  cmdLine->live  = 0;
  cmdLine->background   = 0;
  cmdLine->interval     = 1;
  cmdLine->intervalType = IVAL_HOUR;
  cmdLine->usePigeMeta  = 0;
  cmdLine->useNumbers   = 0;
  cmdLine->skipSongs    = 0;
  cmdLine->useGUI       = 0;
  
  cmdLine->logFile    = "cpige.log";
  cmdLine->dir        = NULL;
  cmdLine->Next       = "A suivre";
  
  for (i = 1; i < argc; i++)
  {
    c = strchr("c", argv[i][1]);
    if (c != NULL)
    {
      /* from streamripper */
      if ((i == (argc-1)) || (argv[i+1][0] == '-'))
      {
        (void)print_help();
        fprintf(stderr, "option %s requires an argument\n", argv[i]);
        _exit(1);
      }
    }
    
    switch (argv[i][1])
    {
      case 'V':
        printf("%s\n", USER_AGENT);
        _exit(0);
        break;

#ifndef NOCONFIG
      case 'c':
        i++;
        configFile = strdup(argv[i]);
        break;
      case 'g':
        cmdLine->useGUI = 1;
        cmdLine->quiet  = 1;
        break;
#else
      case 'h':
        i++;
        
        if (serversPool == NULL)
          serversPool = addPoolUrl(NULL, argv[i]);
        else
          addPoolUrl(serversPool, argv[i]);

        break;
        
      case 'd':
        i++;
        {
          int len = strlen(argv[i]);
          cmdLine->dir = (char *)memory_allocation(len+2);
          strncpy(cmdLine->dir, argv[i], len);

          /* TODO: Windows compatibility ? */
          if (cmdLine->dir[len-1] != '/')
          {
            cmdLine->dir[len] = '/';
            cmdLine->dir[len+1] = 0;
          }
        }
        break;
#ifndef WIN32      
      case 'n':
        cmdLine->useNumbers = 1;
        break;
#endif        
      case 'l':
        i++;
        cmdLine->logFile = strdup(argv[i]);
        break;
        
      case 'P':
        if (cmdLine->live == 1)
        {
          printf("You can't use Live Mode and Pige mode simultaneously.\n");
          _exit(-1);
        }
        if (cmdLine->pige == 1)
          break;

        cmdLine->pige = 1;
        printf("Pige Mode activated.\n");
        break;
        
      case 'q':
        cmdLine->quiet = 1;
        break;
      
      case 'b':
        cmdLine->quiet = 1;
        cmdLine->background = 1;
        break;
      
      case 'L':
        if (cmdLine->pige == 1)
        {
          printf("You can't use Live Mode and Pige mode simultaneously.\n");
          _exit(-1);
        }
        cmdLine->live = 1;
        printf("Live Mode activated.\n");
        break;
        
      case 'M':
        if (cmdLine->pige == 1)
          cmdLine->usePigeMeta = 1;
        
        break;
        
      case 'I':
        i++;

        if (cmdLine->pige != 1)
        {
          cmdLine->pige = 1;
          printf("Pige Mode activated.\n");
        }

        if ( *argv[i] == 'h' || *argv[i] == 'H' )
          cmdLine->intervalType = IVAL_HOUR;
        else if ( *argv[i] == 'm' || *argv[i] == 'M' )
        {
          cmdLine->intervalType = IVAL_MIN;
          if (cmdLine->interval == 1)
            cmdLine->interval = 30;
        }
        else
        {
          fprintf(stderr, "Unknown interval type.\n");
          _exit(1);
        }
        break;
      
      case 'i':
        i++;
       
        if (cmdLine->pige != 1)
        {
          cmdLine->pige = 1;
          printf("Pige Mode activated.\n");
        }
        
        cmdLine->interval = atoi(argv[i]);
        if (cmdLine->interval == 0)
        {
          fprintf(stderr, "Invalid interval 0.\n");
          _exit(1);
        }
        
        break;
        
#endif /* NOCONFIG */

      default:
        fprintf(stderr, "Unknown switch: `%s'\n", argv[i]);
        break;
    } /* switch */
  } /* for */

#ifndef NOCONFIG
  /* Use the config file parser :) */
  
  if (configFile == NULL)
    configFile = strdup("./cpige.conf");
  
  /* Configfile Readin */
  printf("Reading config file: %s\n", configFile);
  if ( (conf = parseConfig( configFile )) == NULL)
  {
    fprintf(stderr, "Could not read config from: %s.\n", configFile);
    _exit(0);
  }

  /* Setting up url pool */
  workconf = conf;
  while (workconf != NULL)
  {
    workconf = _conf_getValue(workconf, "url", &buffer);
    if (buffer == NULL)
      break;
    else
    {
      VERBOSE("Adding %s to the pool.", buffer);
      if (serversPool == NULL)
        serversPool = addPoolUrl(NULL, buffer);
      else
        addPoolUrl(serversPool, buffer);
      free(buffer);
    }
  }
  /* Setting up cpige common parameters */
  set_str_from_conf(conf, "savedirectory", &buffer, NULL, "savedirectory not found in config file.\n", 1);
  {
    int len = strlen(buffer);
    cmdLine->dir = (char *)memory_allocation(len+2);
    strncpy(cmdLine->dir, buffer, len);
    free(buffer);

    /* TODO: Windows compatibility ? */
    if (cmdLine->dir[len-1] != '/')
    {
      cmdLine->dir[len] = '/';
      cmdLine->dir[len+1] = 0;
    }
  }
  
  /* String values */
#ifndef WIN32
  set_str_from_conf(conf,  "pidfile", &(cmdLine->pidFile), "/var/run/cpige.pid", "Warning: no pid file defined. Using /var/run/cpige.pid\n", 0);
#endif
  set_str_from_conf(conf,  "logfile",   &(cmdLine->logFile), "./cpige.log", NULL, 0);
  set_str_from_conf(conf,  "nexttitle", &(cmdLine->Next),    "A suivre",    NULL, 0);

#ifndef WIN32
  set_str_from_conf(conf,  "locale", &buffer, "C", NULL, 0);

  if (setlocale(LC_TIME, buffer) == NULL)
    _ERROR("Error setting up locale: `%s'.\n", buffer);
  free(buffer);
#endif

  /* Int values */
  set_int_from_conf(conf, "cutdelay",  &(cmdLine->interval),   30, NULL, 0);
  set_int_from_conf(conf, "skipsongs", &(cmdLine->skipSongs),  0,  NULL, 0);
  set_str_from_conf(conf, "cuttype",   &buffer,               "h", NULL, 0);
  if ( (*buffer == 'h') || (*buffer == 'H') )
    cmdLine->intervalType = IVAL_HOUR;
  else if ( (*buffer == 'm') || (*buffer == 'M') )
  {
    cmdLine->intervalType = IVAL_MIN;
    if (cmdLine->interval == 1)
      cmdLine->interval = 30;
  } else {
    fprintf(stderr, "Unknown interval type: `%s'. Should be 'm' or 'h'\n", buffer);
    _exit(1);
  }
  free(buffer);

  set_str_from_conf(conf, "timetostop", &buffer, NULL, NULL, 0);
  if (buffer != NULL)
  {
    /* Feature activated */
    for (i = 0; i < strlen(buffer); i++)
    {
      if (! isdigit(buffer[i]))
      {
        /* We have a letter */
        char c = buffer[i];
        buffer[i] = 0;

        if (c == 'h')
        {
          cmdLine->timeToStop = atoi(buffer) * 3600;
        } else if (c == 'm') {
          cmdLine->timeToStop = atoi(buffer) * 60;
        } else if (c == 's') {
          cmdLine->timeToStop = atoi(buffer);
        } else {
          fprintf(stderr, "Unknown interval for timetostop (%c)\n", c);
        }
        
        fprintf(stdout, "Setting timetostop: %lld\n", cmdLine->timeToStop);
        break;
      }
    }
    free(buffer);
  } else {
    cmdLine->timeToStop = -1;
  }

  /* Boolean values */
  set_bool_from_conf(conf, "weekbackup", &(cmdLine->weekBackup),  0, NULL, 0);
  set_bool_from_conf(conf, "pigemode",   &(cmdLine->pige),        0, NULL, 0);
  set_bool_from_conf(conf, "pigemeta",   &(cmdLine->usePigeMeta), 1, NULL, 0);
  if (cmdLine->useGUI != 1)
  {
    set_bool_from_conf(conf, "quietmode",  &(cmdLine->quiet),       0, NULL, 0);
    set_bool_from_conf(conf, "background", &(cmdLine->background),  0, NULL, 0);
  }
  set_bool_from_conf(conf, "usenumbers", &(cmdLine->useNumbers),  0, NULL, 0);

  /* TODO: I know there's a little memleak there,
   * as we are not freeing the conf chained list,
   * and not freeing var(s) / val(s)
   */
  if (cmdLine->background)
    cmdLine->quiet = 1;

#endif /* Using config file */

  if (cmdLine->dir == NULL)
    cmdLine->dir = strdup("./");
 
  _conf_freeConfig(conf);
  /* Don't forget to free it on exit ! */
  return cmdLine;
}

int getCloserInterval(int now, int interval)
{
  int tmp;
  tmp = (now % interval);
  return (now - tmp);
}

int mustCut(lastCut *cut)
{
  int ret = 0;
  int closer;
  
  if (cmdLine->intervalType == IVAL_HOUR)
  {
    closer = getCloserInterval(getHour(), cmdLine->interval);
    if (closer != cut->hour)
      ret = 1;
  } else {
    closer = getCloserInterval(getMinute(), cmdLine->interval);
    if (closer != cut->min)
      ret = 1;
  }
  
  /*
   * if (ret)
   * printf("must cut will return true: itype: %d ival: %d closer: %d hour: %d min: %d\n", cmdLine->intervalType, cmdLine->interval, closer, cut->hour, cut->min);
   */
  return ret;
}

/* Returns what song number we lastly saved
 * the stream to
 */

#ifndef WIN32
int getSongs(char *dir)
{
  DIR *dirp;
  char *filename;
  static regex_t *match = NULL;
  int songs = 0;
  int current = 0;
  struct dirent *cur_dir;
  
  if (match == NULL)
  {
    match = (regex_t *)malloc(sizeof(regex_t));
    if (regcomp(match, "^[0-9]+.*\\.(mp3|aac)$", REG_EXTENDED) != 0)
    {
      _ERROR("Regex compilation error... Please contact developper!\n");
      return 0;
    }
  }

  if ( (dirp = opendir(dir)) == NULL)
  {
    _ERROR("Unable to open directory: %s\n", strerror(errno));
    return 0;
  }

  while ((cur_dir = readdir(dirp)) != NULL)
  {
    filename = cur_dir->d_name;
    
    if ((*filename == '.') || (strcmp(filename, "..") == 0))
      continue;

    if (regexec(match, filename, 0, 0, 0) == 0)
    {
      /* Match */
      current = atoi(filename); /* A bit uggly.. */
      if (current > songs)
        songs = current;
    }
  }
  
  closedir(dirp);

  return songs;
}
#endif

int checkInterval()
{
  int ret = 0;
  if (cmdLine->intervalType == IVAL_HOUR)
  {
    if ((cmdLine->interval <= 0) || (cmdLine->interval > 12))
      ret = 1;
    
  } else if (cmdLine->intervalType == IVAL_MIN) {
    if ((cmdLine->interval <= 0 || cmdLine->interval > 30))
      ret = 1;
  } else {
    ret = 1;
    fprintf(stderr, "Intenal Error: intervalType unknown!\n");
  }
  
  return ret;
}

void testCommandLine()
{
  if (cmdLine == NULL)
  {
    (void) print_help();
    _exit(-1);
  }

  if (serversPool == NULL)
  {
    (void) print_help();
    _exit(-1);
  }

  if (checkInterval() != 0)
  {
    fprintf(stderr, "Incorrect interval specified. Exiting...\n");
    _exit(1);
  }
}

/* Tries to create directories for week backup */
void checkWeekBackup()
{
  int i;
  char *dayName;
  char *dirName;
  int dirNameLen;
  time_t when;
  DIR *weekdir;
  
  if ((! cmdLine->weekBackup) || (cmdLine->pige == 0))
    return;

  when = time((time_t *) NULL);
  /* One week => 7 days ! */
  for (i = 0; i < 7; i++)
  {
    dayName = getDayName((time_t)(when+(i*86400)));
    
    dirNameLen = strlen(cmdLine->dir) + strlen(dayName) + 2;
    dirName = (char *) memory_allocation(dirNameLen);
    snprintf(dirName, dirNameLen, "%s%s/", cmdLine->dir, dayName);
    (void)free(dayName);

    /* Create output dir if does not exists! */
    if (( weekdir = opendir(dirName)) == NULL)
    {
      _ERROR("Unable to open %s for writing\n", dirName);
  #ifdef WIN32
      if (mkdir(dirName) != 0) {
  #else
      if (mkdir(dirName, 0755) != 0) {
  #endif
        _ERROR("Failed trying to create %s. Verify rights.\n", dirName);
        fprintf(stderr, "Failed trying to create %s. Verify rights.\n", dirName);
        _exit(-1);
      } else {
        VERBOSE("Created %s\n", dirName);
        /* pige_dir = opendir(cmdLine->dir); */
      }
    } else {
      VERBOSE("Sucessfully opened %s\n", dirName);
      closedir(weekdir);
    }

    (void)free(dirName);
  }
}

#ifdef SOLARIS
int daemon(int nochdir, int noclose)
{
  /* Dummy function, do not use out of cPige ! */
  int pid;

  pid = fork();
  if (pid == 0)
  {
    _exit(0);
  }
  return 0;
}
#endif
