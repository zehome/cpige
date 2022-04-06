#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "configlib.h"

#define MAXLINE 1024

config_t *parseConfig ( char *filename ) 
{
  FILE *configFile;
  int size, i;
  int bufsize = 256;
  char c;
  char *buf;
  config_t *config;
  confObj_t *confObj;

  if ( (configFile = fopen( filename, "r" )) == NULL )
  {
    fprintf(stderr, "Unable to open config file.\n");
    return NULL;
  }
  
  config = (config_t *)calloc(1, sizeof(config_t));
  config->next = NULL;
  config->conf = NULL;

  buf = (char *)calloc(bufsize, 1);
  memset(buf, 0, bufsize);

  i = 0;
  while (feof(configFile) == 0)
  {
    size = fread(&c, 1, 1, configFile);
    if (size <= 0)
    {
      if (feof(configFile))
        break;
      else
      {
        printf("Error reading config file.\n");
        return NULL;
      }
    }

    if (bufsize < i + 64)
    {
      bufsize += 64;
      buf = realloc(buf, bufsize);
    }
   
    switch (c)
    {
      case '\n':
        buf[i] = 0;
        if (_conf_is_valid_line(buf, i))
        {
          if ( (confObj = _conf_parseLine(buf)) != NULL)
            _conf_setValue(config, confObj);
        }
        
        i = 0;
        break;
        
      default:
        buf[i++] = c;
    }
  }
  free(buf); /* Thx dunky */
  return config;
}

/* Warning, the content of the line could be modified by
 * this function.
 */
int _conf_is_valid_line ( char *line, unsigned int size )
{
  int i;
  int state = 0;

  if (size <= 0)
    return 0;
  
  for (i = 0; i < size; i++)
  {
    if (line[i] != ' ')
    {
      if ( ((line[i] >= 'a') && (line[i] <= 'z')) || 
           ((line[i] >= 'A') && (line[i] <= 'Z'))
         )
        break;
      else
        return 0;
    }
  }
  
  for (i = (size - 1); i >= 0; i--)
  {
    if (line[i] == '/')
    {
      if (state == 0)
      {
        state = 1;
      } else {
        line[i] = 0;
      }
    } else if (line[i] == '\"')
      return 1;

  }
  
  return 1;
}

confObj_t *_conf_parseLine( char *line ) 
{
  int i, j, k;
  int linelen, len;
  int quote, space;
  char *buf;
  char c;
  void *ptr;
  confObj_t *confObj;
  
  if ((line == NULL) || (*line == '\0'))
    return NULL;

  linelen = strlen(line) + 1;
  confObj = (confObj_t *)calloc(1, sizeof(confObj_t));
  buf = calloc(linelen, 1);
  
  for (i = 0, quote = 0, space = 0, j = 0; i < linelen; i++) 
  {
    c = line[i];
    
    switch (c)
    {
      case '"':
        quote ^= 1;
        break;

      case '=':
        if (quote)
            continue;
        if (j == 0)
        {
          fprintf(stderr, "CONFIG ERROR: Incorrect Line. Should not start with '='\n");
          free(confObj);
          free(buf);
          return NULL;
        }
        
        buf[j] = 0;
        j = 0;
        len = strlen(buf);
        
        /* Strip ending spaces */
        for (k = len-1; k > 0 && buf[k] == ' '; k--)
          if (buf[k] == ' ') continue;
        
        buf[k+1] = 0;
        confObj->var = strdup(buf);
        break;
        
      default:
        if ((c == ' ') && (space == 0))
          space = 1;
        else if (c != ' ')
          space = 0;
        
        if ((space == 1) && (quote == 0))
          break; /* Discards */
        if ((space == 1) && (j == 0))
          break; /* Discards */

        buf[j++] = c;
    }
  }
  
  if ((j == 0) || (confObj->var == NULL))
  {
    fprintf(stderr, "Invalid entry: variable defined, but without val.\n");
    free(confObj);
    free(buf);
    return NULL;
  }

  buf[j] = 0;
  
  len = strlen(buf);
  
  /* Ugly strip */
  for (k = len-1; k > 0 && buf[k] == ' '; k--);
  
  buf[k+1] = 0;

  len = strlen(buf);
  ptr = buf;
  
  for (k = 0; k < len && buf[k] == ' '; k++) buf++;
 
  confObj->val = strdup(buf);
 
  free(ptr);

  return confObj;
}

config_t *_conf_setValue( config_t *start, confObj_t *confObj )
{
  config_t *work;
  config_t *last;

  if (start == NULL) 
  {
    fprintf(stderr, "Error: config start is NULL.\n");
    return NULL;
  }

  if (start->conf != NULL) 
  {
    work = (config_t *)calloc(1, sizeof(config_t));
    work->next = NULL;
    work->conf = confObj;
    last = start;
    while (last->next != NULL)
      last = last->next;
    last->next = work;
  } else
    start->conf = confObj;

  return start;
}

void conf_setValue( config_t **start, const char *var, const char *val )
{
  confObj_t *obj;
  
  if ((var == NULL) || (val == NULL))
    return;

  if ((*start) == NULL)
  {
    (*start) = (config_t *)calloc(1, sizeof(config_t));
    (*start)->next = NULL;
    (*start)->conf = NULL;
  }

  obj = (confObj_t *)calloc(1, sizeof(confObj_t));
  obj->var = strdup(var);
  obj->val = strdup(val);

  (*start) = _conf_setValue( *start, obj );
}

int mystr_eq(char *s1, char *s2)
{
  int len1, len2;
  if ((s1 == NULL) || (s2 == NULL)) 
    return 0;

  len1 = strlen(s1);
  len2 = strlen(s2);
  
  if (len1 != len2)
    return 0;
  
  if (strncmp(s1, s2, len1) == 0)
    return 1;

  return 0;
}

config_t *_conf_getValue( config_t *start, char *var, char **dest )
{
  while (start != NULL) 
  {
    if ((start->conf == NULL) || (start->conf->var == NULL))
    {
      *dest = NULL;
      return NULL;
    }

    if (mystr_eq(start->conf->var, var) == 1) 
    {
      *dest = strdup(start->conf->val);
      return start->next;
    }
    start = start->next;
  }
  
  *dest = NULL;
  return NULL;
}

void _conf_printConfig ( config_t *start ) 
{
  while (start != NULL) 
  {
    printf("var: `%s' val: `%s'\n", start->conf->var, start->conf->val);
    start = start->next;
  }
}

void _conf_freeConfig( config_t *start ) 
{
  config_t *old;
  
  while (start != NULL)
  {
    if (start->conf != NULL)
    {
      if (start->conf->var != NULL)
        free(start->conf->var);
      if (start->conf->val != NULL)
        free(start->conf->val);

      free(start->conf);
    }
    old = start;
    start = start->next;
    free(old);
  }
}

void _conf_writeConfig( FILE *stream, config_t *config )
{
  char line[MAXLINE];
  char *lastvar = NULL;
  int len;
  
  fwrite("// Configuration file for cPige written using GUI\n", 50, 1, stream);
  fwrite("// http://ed.zehome.com/?page=cpige-en\n", 39, 1, stream);
  
  while (config != NULL)
  {
    if (config->conf != NULL)
    {
      if ((config->conf->var) && (config->conf->val))
      {
        if (mystr_eq(lastvar, config->conf->var) == 0)
          fwrite("\n", 1, 1, stream);

        len = snprintf(line, MAXLINE, "%s = \"%s\"\n", config->conf->var, config->conf->val);
        fwrite(line, len, 1, stream);
        lastvar = config->conf->var;
      }
    }
    
    config = config->next;
  }
}

void set_str_from_conf(config_t *config, char *type, char **value, char *def, char *errMsg, int exit_n)
{
  _conf_getValue(config, type, value);

  if (*value == NULL) 
  {
  	if (errMsg) {
	    fprintf(stderr, errMsg);
	}
    if (def != NULL)
      *value = strdup(def);
    if (exit_n > 0)
    {
      fprintf(stderr, "Will quit with exit code %d\n", exit_n);
      exit(exit_n);
    }
  }
}

void set_int_from_conf(config_t *config, char *type, int *value, int def, char *errMsg, int exit_n)
{
  char *tmp;
  _conf_getValue(config, type, &tmp);
  
  if ( tmp == NULL )
  {
    fprintf(stderr, errMsg);
    *value = def;
    if (exit_n > 0)
    {
      fprintf(stderr, "Will quit with exit code %d\n", exit_n);
      exit(exit_n);
    }
  } else {
    *value = atoi(tmp);
    free(tmp);
  }
}

void set_bool_from_conf(config_t *config, char *type, int *value, int def, char *errMsg, int exit_n)
{
  char *tmp;
  _conf_getValue(config, type, &tmp);
  
  if ( tmp == NULL )
  {
    fprintf(stderr, errMsg);
    *value = def;
    if (exit_n > 0)
    {
      fprintf(stderr, "Will quit with exit code %d\n", exit_n);
      exit(exit_n);
    }
  } else {
    *value = atoi(tmp);
    if ( (*value) != 1)
      *value = 0;
    free(tmp);
  }
}
