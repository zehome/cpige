#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>

#ifndef WIN32
  #include <signal.h>
  #include <sys/types.h>
#else
  #include <windows.h>
  #include <process.h>
#endif

#include "interface.h"
#include "callbacks.h"
#include "support.h"
#include "main.h"

/* cPige */
#include "configlib.h"

GtkWidget *mainWindow;
gint std_out, std_err;
gboolean cpige_running;
#ifndef WIN32
  GPid pid = -1;
#else
  GPid pid;
#endif

int
main (int argc, char **argv)
{
  GtkTreeView *URLTree;
  GtkToggleButton *tbutton;
  GtkComboBox *combo;
  
  /* Locale config */
  setlocale(LC_ALL, "");
  gtk_set_locale ();
  gtk_init (&argc, &argv);

  /* gettext */
  bindtextdomain("cpige", "share/locale" );
  textdomain("cpige");
  bind_textdomain_codeset("cpige", "UTF-8");
  
  mainWindow = create_mainWindow ();

  /* URL Tree view ! */
  URLTree = GTK_TREE_VIEW(lookup_widget(mainWindow, "URLTreeView"));
  setupURLTreeView(URLTree);
  
  /* Setup default parameters */
  tbutton = GTK_TOGGLE_BUTTON(lookup_widget(mainWindow, "pigeModeButton"));
  gtk_toggle_button_set_active(tbutton, TRUE);
  
  combo = GTK_COMBO_BOX(lookup_widget(mainWindow, "IntervalTypeComboBox"));
  gtk_combo_box_set_active(combo, 1); /* Minute is the default */
 
  if ((argc >= 2) && (argc < 3))
  {
    config_t *conf;

    conf = parseConfig(argv[1]);
    if (! conf)
    {
      ErrorBox(mainWindow,
               gettext("Error parsing config file.\n"
                       "Aborting.\n")
      );
    } else {
      g_object_set_data(G_OBJECT(mainWindow), "_conf_filename", argv[1]);
      if (! setConfig(conf, mainWindow))
      {
        ErrorBox(mainWindow,
                 gettext("Configuration file is invalid.\n"
                         "Aborting.\n")
        );
        g_object_set_data(G_OBJECT(mainWindow), "_conf_filename", NULL);
      }
      _conf_freeConfig( conf ); /* Avoid leak ;) */
    }
  }
  
  gtk_widget_show (mainWindow);
  gtk_main ();
  
  return 0;
}

void addURL(GtkTreeView *tree, gchar *url)
{
  GtkTreeModel *ListStore;
  GtkTreeIter iter;
  gint reco;
  
  /* Initialize the URLTree Widget */
  if ( (ListStore = GTK_TREE_MODEL(gtk_tree_view_get_model(tree))) == NULL)
  {
    ListStore = GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));
    gtk_tree_view_set_model(tree, ListStore);
  }

  /* Create the list store space */
  gtk_list_store_append(GTK_LIST_STORE(ListStore), &iter);

  gtk_list_store_set(GTK_LIST_STORE(ListStore), &iter,
                     0, url,
                    -1);

}

void
setupURLTreeView(GtkTreeView *tree)
{
  GtkCellRenderer *CellRenderer;
  GtkTreeViewColumn *Column;
  GtkTreeModel *ListStore;
  
  /* No headers */
  gtk_tree_view_set_headers_visible(tree, FALSE);

  /* Disable search */
  gtk_tree_view_set_enable_search(tree, FALSE);

  /* Setup the model if none found. */
  if ( (ListStore = GTK_TREE_MODEL(gtk_tree_view_get_model(tree))) == NULL)
  {
    ListStore = GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING));
    gtk_tree_view_set_model(tree, ListStore);
  }

  /* Creates the first column */
  CellRenderer = gtk_cell_renderer_text_new();
  g_object_set(CellRenderer, "editable", TRUE, NULL);
  Column = gtk_tree_view_column_new_with_attributes("URL",
      CellRenderer,
      "text", 0,
      NULL);

  gtk_tree_view_column_set_resizable(Column, TRUE);
  gtk_tree_view_column_set_min_width(Column, 350);
  /* Connect the "Editable Cell renderer" */
  g_signal_connect(CellRenderer, "edited", G_CALLBACK(url_edited_callback), ListStore);
  
  /* Add the column */
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), Column);  

  /* Create the second column */
  /* CellRenderer = gtk_cell_renderer_text_new();
   * Column = gtk_tree_view_column_new_with_attributes("Reco",
   *   CellRenderer,
   *   "text", 1,
   *   NULL);
   *
   * g_object_set (G_OBJECT(CellRenderer), "xalign", 1.0, NULL);
   */
  /* Align right */
  /*gtk_tree_view_column_set_alignment(Column, 1.0); */

  /* Add the column */
  /* gtk_tree_view_append_column(GTK_TREE_VIEW(tree), Column);  */
  
}

/* This function will build a config tree 
 * for use with cPige.
 */

void ClearUrlTreeView(GtkTreeView *tree)
{
  GtkTreeModel *model;
  
  model = GTK_TREE_MODEL(gtk_tree_view_get_model(tree));
  
  if (! model)
    return;
  gtk_list_store_clear(GTK_LIST_STORE(model));
}

void newConfig()
{
  config_t *conf = NULL;
  conf_setValue(&conf, "nourl",           "1"); /* Hack */
  conf_setValue(&conf, "savedirectory",   "./");
  conf_setValue(&conf, "weekbackup",      "0");
  conf_setValue(&conf, "pigemode",        "0");
  conf_setValue(&conf, "pigemeta",        "1");
  conf_setValue(&conf, "quietmode",       "0");
  conf_setValue(&conf, "background",      "0");
  conf_setValue(&conf, "usenumbers",      "0");
  conf_setValue(&conf, "skipsongs",       "0");
  conf_setValue(&conf, "nexttitle",       "A Suivre:");
  conf_setValue(&conf, "cuttype",         "m");
  conf_setValue(&conf, "cutdelay",        "30");
  conf_setValue(&conf, "logfile",         "cpige.log");
  setConfig(conf, mainWindow);
  _conf_freeConfig(conf);
}

/* setup a cPige config to the GUI */
int setConfig(config_t *conf, GtkWidget *mainWindow)
{
  GtkWidget *widget;
  gchar *val, *new;
  gint intval, retval = FALSE;
  config_t *work;

  /* Setup URLs */
  widget = lookup_widget(mainWindow, "URLTreeView");
  ClearUrlTreeView(GTK_TREE_VIEW(widget));
 
  /* This is a hack to avoid the return ;) */
  _conf_getValue(conf, "nourl", &val);
  if (! val)
  { 
    work = conf;
    while (work != NULL)
    {
      work = _conf_getValue(work, "url", &val);
      if (val)
      {
        retval = TRUE;
        (void)addURL(GTK_TREE_VIEW(widget), val);
      }
    }

    if (retval == FALSE)
      return retval;
  } else {
    free(val);
  }

  /* Setup savedirectory */
  widget = lookup_widget(mainWindow, "FileChooserButton");
  set_str_from_conf(
      conf, 
      "savedirectory", 
      &val, 
      NULL, 
      "savedirectory not found in config file.\n",
      0
  );
  
  if (val == NULL)
  {
    ErrorBox(mainWindow, gettext("Invalid config file: no savedirectory specified.\n"));
    return FALSE;
  }

  if (val[(strlen(val)-1)] != '/')
  {
    realloc(val, strlen(val)+2);
    g_strlcat(val, "/", strlen(val)+2);
  }

  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widget), val);

  /* Setup weekbackup */
  widget = lookup_widget(mainWindow, "WeekBackupButton");
  set_int_from_conf(
      conf,
      "weekbackup",
      &intval,
      0,
      NULL,
      0
  );
      
  if (intval == 1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);

  /* Setup Pige Mode */
  widget = lookup_widget(mainWindow, "pigeModeButton");
  set_int_from_conf(
      conf,
      "pigemode",
      &intval,
      0,
      NULL,
      0
  );

  if (intval == 1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);

  /* Setup Pige Meta */
  widget = lookup_widget(mainWindow, "pigeMetaButton");
  set_int_from_conf(
      conf,
      "pigemeta",
      &intval,
      1,
      NULL,
      0
  );
      
  if (intval == 1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);

  /* Setup Quiet Mode */
  widget = lookup_widget(mainWindow, "quietModeButton");
  set_int_from_conf(
      conf,
      "quietmode",
      &intval,
      0,
      NULL,
      0
  );
      
  if (intval == 1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);

  /* Setup Background Mode */
  widget = lookup_widget(mainWindow, "backgroundButton");
  set_int_from_conf(
      conf,
      "background",
      &intval,
      0,
      NULL,
      0
  );
      
  if (intval == 1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);

  /* Setup usenumbers */
  widget = lookup_widget(mainWindow, "useNumbersButton");
  set_int_from_conf(
      conf,
      "usenumbers",
      &intval,
      0,
      NULL,
      0
  );
      
  if (intval == 1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
  else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
  
  /* Setup skipSongs */
  widget = lookup_widget(mainWindow, "skipSongsSpin");
  set_int_from_conf(
      conf,
      "usenumbers",
      &intval,
      0,
      NULL,
      0
  );
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), (gdouble)(intval));
 
  /* Setup next title */
  widget = lookup_widget(mainWindow, "NextTitleEntry");
  set_str_from_conf(
      conf, 
      "nexttitle", 
      &val,
      "A Suivre:",
      NULL,
      0
  );
  gtk_entry_set_text(GTK_ENTRY(widget), val);

  /* Setup cuttype */
  widget = lookup_widget(mainWindow, "IntervalTypeComboBox");
  set_str_from_conf(conf, "cuttype", &val, "h", NULL, 0);
 
  if ((*val == 'h') || (*val == 'H'))
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 0);
  else
    gtk_combo_box_set_active(GTK_COMBO_BOX(widget), 1);

  /* Setup cutdelay */
  widget = lookup_widget(mainWindow, "IntervalDelaySpinButton");
  set_int_from_conf(
      conf,
      "cutdelay",
      &intval,
      1,
      NULL,
      0
  );
  
  free(val);

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), (gdouble)(intval));

  /* Setup logfile */
  widget = lookup_widget(mainWindow, "LogfileEntry");
  set_str_from_conf(conf, "logfile", &val, "cpige.log", NULL, 0);
  gtk_entry_set_text(GTK_ENTRY(widget), val);
  
  return TRUE;
}
config_t *buildConfig()
{
  /*
   * Parameters that will be get:
   * 
   *  url                ok
   *  savedirectory      ok
   *  weekbackup         ok
   *  logfile
   *  pigemode           ok
   *  pigemeta           ok
   *  cuttype            ok
   *  cutdelay           ok
   *  quietmode          ok
   *  background         ok
   *  pidfile
   *  skipsongs
   *  nexttitle          ok
   * 
   */
  GtkWidget *widget;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *val;
  gint intval;
  config_t *conf = NULL;
  
  /* Retreiving the URL */
  widget = lookup_widget(mainWindow, "URLTreeView");
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));

  if ( (! model) || (! gtk_tree_model_get_iter_first(model, &iter)))
  {
    ErrorBox(mainWindow, gettext("Invalid configuration: no URL specified.\n"));
    return NULL;
  } else {
    do
    {
      gtk_tree_model_get(model, &iter, 0, &val, -1);
      conf_setValue(&conf, "url", val);
      g_free(val);
    } while ( gtk_tree_model_iter_next(model, &iter) == TRUE);
  }

  /* Retreiving savedirectory */
  widget = lookup_widget(mainWindow, "FileChooserButton");
  val = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));  
  
  conf_setValue(&conf, "savedirectory", val);
  g_free(val);

  /* Retreiving weekbackup */
  widget = lookup_widget(mainWindow, "WeekBackupButton");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
    val = "1";
  else
    val = "0";
  conf_setValue(&conf, "weekbackup", val);
  /* No need to free val there */

  /* Retreiving Pige Mode */
  widget = lookup_widget(mainWindow, "pigeModeButton");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
    val = "1";
  else
    val = "0";
  conf_setValue(&conf, "pigemode", val);

  /* Retreiving Pige Meta */
  widget = lookup_widget(mainWindow, "pigeMetaButton");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
    val = "1";
  else
    val = "0";
  conf_setValue(&conf, "pigemeta", val);

  /* Retreiving Quiet Mode */
  widget = lookup_widget(mainWindow, "quietModeButton");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
    val = "1";
  else
    val = "0";
  conf_setValue(&conf, "quietmode", val);

  /* Retreiving Background Mode */
  widget = lookup_widget(mainWindow, "backgroundButton");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
    val = "1";
  else
    val = "0";
  conf_setValue(&conf, "background", val);

  /* Retreiving usenumbers */
  widget = lookup_widget(mainWindow, "useNumbersButton");
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == TRUE)
    val = "1";
  else
    val = "0";
  conf_setValue(&conf, "usenumbers", val);
  
  /* Retreiving skipSongs */
  widget = lookup_widget(mainWindow, "skipSongsSpin");
  intval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  val = (gchar *)g_malloc(8);
  snprintf(val, 8, "%d", intval);
  conf_setValue(&conf, "skipsongs", val);
  g_free(val);
 
  /* Retreiving next title */
  widget = lookup_widget(mainWindow, "NextTitleEntry");
  val = (gchar *)gtk_entry_get_text(GTK_ENTRY(widget));
  /* We must not free val, it's used by GTK internaly. */
  conf_setValue(&conf, "nexttitle", val);

  /* Retreiving cuttype */
  widget = lookup_widget(mainWindow, "IntervalTypeComboBox");
  if (gtk_combo_box_get_active(GTK_COMBO_BOX(widget)) == 0)
    conf_setValue(&conf, "cuttype", "h");
  else
    conf_setValue(&conf, "cuttype", "m");

  /* Retreiving cutdelay */
  widget = lookup_widget(mainWindow, "IntervalDelaySpinButton");
  intval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
  val = (gchar *)g_malloc(8);
  snprintf(val, 8, "%d", intval);
  conf_setValue(&conf, "cutdelay", val);
  g_free(val);
  
  /* Retreiving logfile */
  widget = lookup_widget(mainWindow, "LogfileEntry");
  val = (gchar *)gtk_entry_get_text(GTK_ENTRY(widget));
  conf_setValue(&conf, "logfile", val);

  return conf;
}

gboolean is_cpige_running()
{
  return cpige_running;
}

gboolean launch_cpige()
{
  gchar *config;
  GIOChannel *channel;
  GError *error = NULL;
  gboolean ok;
  char *cpigePath;

  config = g_object_get_data(G_OBJECT(mainWindow), "_conf_filename");
  if (! config)
  {
    g_printf("No config file specified.\n");
    return FALSE;
  }

  gchar *argv[5] = { "cpige", "-g", "-c", config, NULL };

  cpigePath = g_find_program_in_path("cpige");

  if (cpigePath == NULL)
  {
    ErrorBox(mainWindow,
              gettext("Unable to find program cpige. Have you renamed cpige to another filename?\n"));
    return FALSE;
  }

  /* Cool, now we could spawn cPige */

  ok = g_spawn_async_with_pipes(NULL, 
                           argv,
                           NULL,
                           G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
                           NULL,
                           NULL,
                           &pid,
                           NULL,      /* stdin  */
                           &std_out,  /* stdout */
                           NULL,      /* stderr */
                           &error);
  if (! ok)
  {
    ErrorBox(mainWindow, error->message);
    g_warning(G_STRLOC ": %s", error->message);
    return FALSE;
  }
#ifdef WIN32
  if (pid == NULL || pid == INVALID_HANDLE_VALUE)
  {
    printf("BadHandle detected. (pid: %p) Error: %lu.\n", pid, GetLastError());
    fflush(stdout);
    return FALSE;
  }

#endif
  channel = g_io_channel_unix_new(std_out);
  g_io_add_watch(channel,
                 G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_NVAL | G_IO_ERR,
                 &cPigeEvent,
                 NULL); /* There we should specify which cPige has raised the event */
  g_io_channel_unref(channel);
  
  cpige_running = TRUE;

  return TRUE;
}


void kill_cpige()
{
  if (! is_cpige_running())
    return;

#ifndef WIN32
  if (pid != -1)
    kill(pid, SIGKILL);
#else
  if (! TerminateProcess((HANDLE)pid, 0))
  {

    LPVOID lpMsgBuf; 
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
       FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
    MessageBox(NULL,(LPCTSTR )lpMsgBuf, "Erreur", MB_OK | MB_ICONERROR);
    LocalFree(lpMsgBuf);
  } else {
    printf("Should have killed cpige...\n");
    fflush(stdout);
  }

#endif
  g_spawn_close_pid(pid);

#ifndef WIN32
  pid = -1;
#else
  pid = NULL;
#endif
  cpige_running = FALSE;
}
