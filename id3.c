#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tool.h"
#include "id3.h"

char *GetId3v1(char *titre, char *artiste, char *radioName)
{
  char *chunk;
  char *relativePointer;
  int padding = 0;
  
  chunk = (char *)memory_allocation(128);
  
  /* id3v1 tag */
  strncat(chunk, "TAG", 4);
  
  /* Title */
  relativePointer = chunk + 3; /* 3 octets */
  if (titre) {
    padding = (30 - strlen(titre));
    if (padding < 0)
    {
      snprintf(relativePointer+3, 31, "%s", titre);
    } else {
      snprintf(relativePointer, 128-3, "%s", titre);
      memset(relativePointer + (30-padding), 0, padding);
    }
  } else {
    memset(relativePointer, 0, 30);
  }
  
  /* Artist */
  relativePointer = relativePointer + 30; /* 33 octets */
  if (artiste) {
    padding = (30 - strlen(artiste));
    if (padding < 0) {
      snprintf(relativePointer, 31, "%s", artiste);
    } else {
      snprintf(relativePointer, 128-33, "%s", artiste);
      memset(relativePointer + (30 - padding), 0, padding);
    }
  } else {
    memset(relativePointer, 0, 30);
  }
  
  /* Album (on met l'url de la radio ...) */
  relativePointer = relativePointer + 30; /* 63 octets */
  if (radioName != NULL) {
    padding = (30 - strlen(radioName));
    if (padding < 0) {
      snprintf(relativePointer, 31, "%s", radioName);
    } else {
      snprintf(relativePointer, 128-63, "%s", radioName);
      memset(relativePointer + (30-padding), 0, padding);
    }
  } else {
    memset(relativePointer, 0, 30);
  }
  
  /* Year */
  relativePointer = relativePointer + 30; /* 93 octets */
  memset(relativePointer, 0, 4);
  
  /* Comment */
  relativePointer = relativePointer + 4; /* 97 octets */
  snprintf(relativePointer, 31, "by cPige http://ed.zehome.com/"); 
  /* Ouah la chance, ça rentre! */
  
  /* genre */
  relativePointer = relativePointer + 30; /* 127 octets */
  memset(relativePointer, 1, 1);
  
  /* 128 bytes ! We won :) */
  return chunk;  
}
