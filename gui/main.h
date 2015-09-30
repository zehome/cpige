#ifndef __CPIGE_GUI_MAIN_H
#define __CPIGE_GUI_MAIN_H

#include "configlib.h"
#include <gtk/gtk.h>

void setupURLTreeView(GtkTreeView *tree);
void addURL(GtkTreeView *tree, gchar *url);
int setConfig(config_t *conf, GtkWidget *mainWindow);
gboolean launch_cpige();
config_t *buildConfig();
void ClearUrlTreeView(GtkTreeView *tree);

#endif
