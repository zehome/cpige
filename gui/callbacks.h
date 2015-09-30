#include <gtk/gtk.h>


void
on_nouveau_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_ouvrir_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_enregistrer_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_enregistrer_sous_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quitter_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_FileChooserButton_current_folder_changed
                                        (GtkFileChooser  *filechooser,
                                        gpointer         user_data);

void
on_AddUrlBUtton_activate               (GtkButton       *button,
                                        gpointer         user_data);

void
on_DelURLButton_pressed                (GtkButton       *button,
                                        gpointer         user_data);

void
on_IntervalTypeComboBox_changed        (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_pigeModeButton_pressed              (GtkButton       *button,
                                        gpointer         user_data);

void
on_startButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_LogfileCheckButton_pressed          (GtkButton       *button,
                                        gpointer         user_data);

void
on_AddUrlBUtton_pressed                (GtkButton       *button,
                                        gpointer         user_data);

void
on_startButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_log_open_pressed                    (GtkButton       *button,
                                        gpointer         user_data);

void
url_edited_callback                    (GtkCellRendererText *cell,
                                        gchar               *path_string,
                                        gchar               *new_text,
                                        gpointer             user_data);

void something_changed(GtkButton       *button,
                       gpointer         user_data);


gboolean cPigeEvent (GIOChannel *source,
                     GIOCondition condition,
                     gpointer data);
