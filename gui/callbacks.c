#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <libintl.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "configlib.h"
#include "main.h"

extern GtkWidget *mainWindow;

void
on_nouveau_activate                   (GtkMenuItem     *menuitem,
                                       gpointer         user_data)
{
  GtkWidget *dialog;
  gint retour;

  dialog = GTK_WIDGET(create_newConfirmDialog());
  retour = gtk_dialog_run(GTK_DIALOG(dialog));

  switch(retour)
  {
    case GTK_RESPONSE_ACCEPT:
      newConfig();
      break;

    case GTK_RESPONSE_CANCEL:
      break;
  }

  gtk_widget_destroy(dialog);
}


void
on_ouvrir_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *dialog;
  gint retour;
  gchar *filename, *tmp;
  config_t *conf;
  
  dialog = gtk_file_chooser_dialog_new(
              gettext("Open new configuration"),
              GTK_WINDOW(mainWindow),
              GTK_FILE_CHOOSER_ACTION_OPEN,
              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
              GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
              NULL);

  retour = gtk_dialog_run(GTK_DIALOG(dialog));
  switch(retour)
  {
    case GTK_RESPONSE_ACCEPT:
      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      if (filename == NULL)
      {
        ErrorBox(mainWindow,
                 gettext("No config file specified.\n"
                         "Internal Error, aborting.\n")
        );
        break;
      }

      conf = parseConfig(filename);
      if (! conf)
      {
        ErrorBox(mainWindow,
                 gettext("Error parsing config file.\n"
                         "Aborting.\n")
        );
        break;
      }

      g_object_set_data(G_OBJECT(mainWindow), "_conf_filename", filename);
      if (! setConfig(conf, mainWindow))
      {
        ErrorBox(mainWindow,
                 gettext("Configuration file is invalid.\n"
                         "Aborting.\n")
        );
        g_object_set_data(G_OBJECT(mainWindow), "_conf_filename", NULL);
        g_free(filename);
      }
      _conf_freeConfig( conf ); /* Avoid leak ;) */
      break;

    case GTK_RESPONSE_CANCEL:
      break;

  }

  gtk_widget_destroy(dialog);
}


gboolean
on_enregistrer_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *main, *dialog;
  gchar *filename;
  gboolean retval = FALSE;
  config_t *conf;
  FILE *configFile;
  
  main = mainWindow;
  if ( (filename = g_object_get_data(G_OBJECT(main), "_conf_filename")) != NULL)
  {
    conf = buildConfig();
    if (conf ==  NULL)
    {
      ErrorBox(main, gettext("An error was detected during the configuration building.\n"
                             "Settings are not acceptable. Please reconfigure cPige.\n"
                             "This would happens for example if you've not added an URL.\n")
      );
    } else {
      if ( (configFile = (FILE *)g_fopen(filename, "w")) == NULL)
      {
        ErrorBox(main,
            gettext("Unable to open your config file for writing.\n"
                    "Please verify rights.\n")
        );
      } else {
        g_object_set_data(G_OBJECT(main), "_conf_filename", filename);
        _conf_writeConfig( configFile , conf );
        _conf_freeConfig( conf );
        fclose(configFile);
        retval = TRUE;
      }
    }
  } else
    return on_enregistrer_sous_activate(NULL, user_data);

  return retval;
}


gboolean
on_enregistrer_sous_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *saveDialog;
  GtkWidget *dialog;
  GtkWidget *main;
  gint retour;
  gboolean retval = FALSE;
  gchar *filename;
  FILE *configFile;
  config_t *conf;
  
  main = mainWindow;
  conf = buildConfig();
  if (conf ==  NULL)
  {
    ErrorBox(main,
             gettext("An error was detected during the configuration building.\n"
                     "Settings are not acceptable. Please reconfigure cPige.\n"
                     "This would happens for example if you've not added an URL.\n")
    );
    return;
  }
 
  saveDialog = GTK_WIDGET(create_FileSaveDialog());
  retour = gtk_dialog_run(GTK_DIALOG(saveDialog));
  switch (retour)
  {
    case GTK_RESPONSE_OK:
    case GTK_RESPONSE_ACCEPT:
      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(saveDialog));
      if ( (configFile = (FILE *)g_fopen(filename, "w")) == NULL)
      {
        gtk_widget_destroy(GTK_WIDGET(saveDialog));
        ErrorBox(
            main,
            gettext("Unable to open config file for writing.\nSettings will *NOT* be saved.\n")
        );
      } else {
        g_object_set_data(G_OBJECT(main), "_conf_filename", strdup(filename));
        gtk_widget_destroy(GTK_WIDGET(saveDialog));
        _conf_writeConfig( configFile, conf );
        fclose(configFile);
        retval = TRUE;
      }
      
      g_free(filename);
      break;
      
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy(GTK_WIDGET(saveDialog));
      break;
  }

  _conf_freeConfig( conf );

  return retval;
}


void
on_quitter_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkWidget *dialog;
  gint ret;
  gint is_quit = 1;
  
  if (is_cpige_running())
  {
    dialog = gtk_message_dialog_new(
        GTK_WINDOW(mainWindow),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        gettext("Warning, cPige is currently recording.\n"
                "Exiting will shutdown cPige.\n"
                "Do you really want to continue ?\n")
    );

    ret = gtk_dialog_run(GTK_DIALOG(dialog));
    switch(ret)
    {
      case GTK_RESPONSE_YES:
        kill_cpige();
        break;

      case GTK_RESPONSE_NO:
      default:
        is_quit = 0;
        break;
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
  }
  
  if (is_quit)
    exit(0);
}

void
on_AddUrlBUtton_pressed                (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkTreeView *URLTree;
  GtkDialog *addurlDialog;
  GtkEntry *urlEntry;
  gint retour;
  
  addurlDialog = GTK_DIALOG(create_URLAddDialog());
  
  retour = gtk_dialog_run(addurlDialog);
  switch (retour)
  {
    case GTK_RESPONSE_OK:
      URLTree = GTK_TREE_VIEW(lookup_widget(GTK_WIDGET(button), "URLTreeView"));
      urlEntry = GTK_ENTRY(lookup_widget(GTK_WIDGET(addurlDialog), "URLAddEntry"));
      addURL(URLTree, (gchar *)gtk_entry_get_text(urlEntry));
      something_changed(NULL, NULL);
      break;
    default:
      /* No action */
      break;
  }
  gtk_widget_destroy(GTK_WIDGET(addurlDialog));
}

void
on_DelURLButton_pressed                (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkTreeView *URLTree;
  GtkTreeSelection *Selection;
  GtkTreeIter iter;
  GtkListStore *ListStore;

  URLTree = (GtkTreeView *)lookup_widget((GtkWidget *)button, "URLTreeView");
  ListStore = GTK_LIST_STORE(gtk_tree_view_get_model(URLTree));
  if (! ListStore)
  {
    g_printf("No valid ListStore found. Could not remove an item.\n");
    return;
  }
  
  Selection = gtk_tree_view_get_selection(URLTree);

  if (gtk_tree_selection_get_selected (Selection,
                                   NULL,
                                   &iter))
    gtk_list_store_remove(ListStore, &iter);
  
  something_changed(NULL, NULL);
}

void
on_IntervalTypeComboBox_changed        (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
  gint idx;
  GtkSpinButton *spinButton;

  spinButton = GTK_SPIN_BUTTON(lookup_widget(GTK_WIDGET(combobox), "IntervalDelaySpinButton"));
  
  idx = gtk_combo_box_get_active(combobox);
  if (idx == 0) /* Hour */
  {
    gtk_spin_button_set_adjustment(spinButton, GTK_ADJUSTMENT(gtk_adjustment_new (1, 1, 12, 1, 1, 1)));
    gtk_spin_button_set_value(spinButton, 1);
  } else if (idx == 1) { /* Minute */
    gtk_spin_button_set_adjustment(spinButton, GTK_ADJUSTMENT(gtk_adjustment_new (10, 10, 30, 10, 10, 10)));
    gtk_spin_button_set_value(spinButton, 10);
  } else {
    g_printf("Unknown interval type (%d)\n", idx);
  }
}


void
on_pigeModeButton_pressed              (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkFrame *PigeFrame;
  GtkToggleButton *weekbackup, *useNumbers, *pigeMetaButton;
  gboolean state;
  
  PigeFrame      = GTK_FRAME(lookup_widget(GTK_WIDGET(button), "PigeModeFrame"));
  
  weekbackup     = GTK_TOGGLE_BUTTON(lookup_widget(GTK_WIDGET(button), "WeekBackupButton"));
  useNumbers     = GTK_TOGGLE_BUTTON(lookup_widget(GTK_WIDGET(button), "useNumbersButton"));
  pigeMetaButton = GTK_TOGGLE_BUTTON(lookup_widget(GTK_WIDGET(button), "pigeMetaButton"));

  state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
  if (state == FALSE) /* Button *was* active. */
  {
    gtk_widget_set_sensitive(GTK_WIDGET(PigeFrame),      FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(useNumbers),     TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(weekbackup),     FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(pigeMetaButton), FALSE);
  }
  else /* Button *was* inactive */
  {
    gtk_widget_set_sensitive(GTK_WIDGET(PigeFrame),      TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(useNumbers),     FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(weekbackup),     TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(pigeMetaButton), TRUE);
  }
}


void
on_startButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  gboolean state;
  GtkWidget *image;
  GtkWidget *wid;
  GtkWidget *main;
  int i; 
  gchar *DisableFrames[] = {"ParametersFrame", "ExtraParametersFrame", NULL};
  gchar *iter, *filename;
  
  main = mainWindow;

  if ( (filename = g_object_get_data(G_OBJECT(main), "_conf_filename")) == NULL)
  {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
      WarningBox( main,
                  gettext("You must save your configuration before launching cPige.\n")
      );

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    return;
  }
 
  state = (gboolean)g_object_get_data(G_OBJECT(button), "savemode");
  if ((state == TRUE) &&
      (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == TRUE))
  {
    if (on_enregistrer_activate(NULL, user_data) == TRUE)
    {
      gtk_button_set_label(button, gettext("Start"));
      g_object_set_data(G_OBJECT(button), "savemode", FALSE);
    }
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
    return;
  }
 
  image = lookup_widget(GTK_WIDGET(button), "startButtonImage");
  state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
  
  if (state == TRUE)
  {
    if (! launch_cpige())
    {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
      return;
    }

    gtk_button_set_label(button, gettext("Stop"));
    /* Icon gtk-stop */
    gtk_image_set_from_stock(GTK_IMAGE(image), GTK_STOCK_QUIT, GTK_ICON_SIZE_BUTTON);
  } else {
    gtk_button_set_label(button, gettext("Start"));
    /* Icon gtk-media-play */
    gtk_image_set_from_stock(GTK_IMAGE(image), GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
    g_object_set_data(G_OBJECT(mainWindow), "stopping", (void *)NULL);
    kill_cpige();
  }
  gtk_button_set_image(button, image);

  for (i = 0; DisableFrames[i] != NULL; i++)
  {
    wid = lookup_widget(GTK_WIDGET(button), DisableFrames[i]);
    gtk_widget_set_sensitive(wid, !state);
  }
}

void
on_LogfileCheckButton_pressed          (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *lbutton, *entry;
  gboolean state;
  
  lbutton = lookup_widget(GTK_WIDGET(button), "LogfileChooserButton");
  entry = lookup_widget(GTK_WIDGET(button), "LogfileEntry");
  state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
  if (state == FALSE) /* The button was active, and is not inactive */
  {
    gtk_widget_set_sensitive(lbutton, FALSE);
    gtk_widget_set_sensitive(entry, FALSE);
  } else { /* The button was inactive and is now active */
    gtk_widget_set_sensitive(lbutton, TRUE);
    gtk_widget_set_sensitive(entry, TRUE);
  }
  
  something_changed(NULL, NULL);
}

void
on_log_open_pressed                    (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *saveDialog;
  GtkWidget *logentry;
  gint retour;
  gchar *filename;
  FILE *logFile;

  saveDialog = GTK_WIDGET(create_FileSaveDialog());
  retour = gtk_dialog_run(GTK_DIALOG(saveDialog));
  switch (retour)
  {
    case GTK_RESPONSE_OK:
    case GTK_RESPONSE_ACCEPT:
      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(saveDialog));
      if ( (logFile = (FILE *)g_fopen(filename, "w")) == NULL)
      {
        gtk_widget_destroy(GTK_WIDGET(saveDialog));
        ErrorBox(
            mainWindow,
            gettext("Unable to open config file for writing.\nSettings will *NOT* be saved.\n")
        );
      } else {
        gtk_widget_destroy(GTK_WIDGET(saveDialog));
        logentry = lookup_widget(GTK_WIDGET(button), "LogfileEntry");
        gtk_entry_set_text(GTK_ENTRY(logentry), filename);
        something_changed(NULL, NULL);
        fclose(logFile);
      }
      
      g_free(filename);
      break;
      
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy(GTK_WIDGET(saveDialog));
      break;
  }
  
}


void
url_edited_callback                    (GtkCellRendererText *cell,
                                        gchar               *path_string,
                                        gchar               *new_text,
                                        gpointer            model)
{
  GtkTreeIter iter;
  
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(model), &iter, path_string) == FALSE)
  {
    g_printf("Edited wrong cell!!\n");
    return;
  }

  gtk_list_store_set(model, &iter, 0, new_text, -1);
  something_changed(NULL, NULL);
}

void something_changed(GtkButton       *button,
                       gpointer         user_data)
{
  gint state;
  GtkWidget *startButton;
  /* Something in the configuration has changed.
   *
   * cPige GUI will change the Start Button
   * to Save / Start
   */

  /* Retreive start button state */
  startButton = lookup_widget(GTK_WIDGET(mainWindow), "startButton");

  state = (gboolean)g_object_get_data(G_OBJECT(startButton), "savemode");
  if (state == 0)
  {
    /* Button is not in save mode. Switch to save mode ;) */
    g_object_set_data(G_OBJECT(startButton), "savemode", (gpointer)TRUE);
    gtk_button_set_label(GTK_BUTTON(startButton), gettext("Apply"));
  }
}


/* GIOFunc */
gboolean cPigeEvent (GIOChannel *source,
                     GIOCondition condition,
                     gpointer data)
{
  GError *error = NULL;
  GtkWidget *titleWidget, *nextTitleWidget;
  GtkWidget *progress;
  GtkWidget *startButton;
  gchar *line;
  gsize line_len, terminator;
  gchar *title;
  gchar *next_title;
  gchar *left;
  gchar *percent_str;
  if ((condition & G_IO_IN) || (condition & G_IO_PRI))
  {
    /* Lookup widget to update */
    titleWidget     = lookup_widget(GTK_WIDGET(mainWindow), "titleEntry");
    nextTitleWidget = lookup_widget(GTK_WIDGET(mainWindow), "nexttitleEntry");
    progress        = lookup_widget(GTK_WIDGET(mainWindow), "timeleftProgressBar");

    /* there is data to read */
    if (g_io_channel_read_line(source, &line, &line_len, &terminator, &error) != G_IO_STATUS_NORMAL)
    {
      g_warning("Error reading the line ...\n");
      gtk_entry_set_text(GTK_ENTRY(titleWidget),     "");
      gtk_entry_set_text(GTK_ENTRY(nextTitleWidget), "");
      gtk_progress_set_percentage(GTK_PROGRESS(progress), 0.0);
      gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), "timeleft");
      return FALSE;
    }
 
    line[terminator] = '\0';
  
    if ((line_len < 9) || (strncmp((char *)line, "CPIGEGUI", 8) != 0))
    {
      g_free(line);
      return TRUE;
    }

    line += 9;
    line_len -= 9;
    
    title       = (gchar *)strtok(line, ":");
    next_title  = (gchar *)strtok(NULL, ":");
    left        = (gchar *)strtok(NULL, ":");
    percent_str = (gchar *)strtok(NULL, ":");
    
    /* Now, update widgets */
    if (title)
      gtk_entry_set_text(GTK_ENTRY(titleWidget),     title);
    if ((next_title) && (strlen(next_title)))
      gtk_entry_set_text(GTK_ENTRY(nextTitleWidget), next_title);
    
    if ((left == NULL) || (*left == 0))
      gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress));
    else
    {
      if ((percent_str) && (strlen(percent_str)))
      {
        int percentage, hour, min, sec, left_sec;
        gchar statusText[100];
        memset(statusText, 0, 100);
        
        left_sec = atoi(left);
        hour = left_sec / 3600;
        min  = (left_sec / 60) - (hour*60);
        sec  = left_sec - (hour*3600) - (min*60);
        percentage = atoi(percent_str);
        gtk_progress_set_percentage(GTK_PROGRESS(progress), ((float)percentage / 100.0));
        snprintf(statusText, 100, "%dh %dm %ds (%d%%)", hour, min, sec, percentage);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), statusText);
      }
    }

    g_free(line);
    
  } else if (condition & G_IO_HUP) {
    if (g_object_get_data(G_OBJECT(mainWindow), "stopping") != NULL)
      g_object_set_data(G_OBJECT(mainWindow), "stopping", NULL);
    else
      ErrorBox(mainWindow, gettext("cPige exited.\n")); 
    
    startButton = GTK_WIDGET(lookup_widget(GTK_WIDGET(mainWindow), "startButton"));
    gtk_button_set_label(GTK_BUTTON(startButton), gettext("Start"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(startButton), FALSE);

    return FALSE;
  } else {
    g_printf("Uncatched event received: %d\n", condition);
    return FALSE;
  }
}
