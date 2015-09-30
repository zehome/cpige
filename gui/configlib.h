#ifndef __CONFIGLIB_H
#define __CONFIGLIB_H

typedef struct
{
  char *var;
  char *val;
} confObj_t;

/* Base of the chained list */
typedef struct config_t
{
  confObj_t       *conf;
  struct config_t *next;
} config_t;

config_t *parseConfig ( char *filename );
config_t *_conf_setValue( config_t *start, confObj_t *confObj );
confObj_t *_conf_parseLine( char *line );

config_t *_conf_getValue( config_t *start, char *var, char **dest);
void _conf_freeConfig( config_t *start );
void _conf_printConfig ( config_t *start );
void _conf_writeConfig( FILE *stream, config_t *config );

int _conf_is_valid_line ( char *line, unsigned int size );

/* Trem's contribution */

void set_str_from_conf(config_t *config, char *type, char **value, char *def, char *errMsg, int exit_n);

void set_int_from_conf(config_t *config, char *type, int *value, int def, char *errMsg, int exit_n);

/* End of trem's contrib! */

void set_bool_from_conf(config_t *config, char *type, int *value, int def, char *errMsg, int exit_n);

#endif
