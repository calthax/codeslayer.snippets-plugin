/*
 * Copyright (C) 2010 - Jeff Johnston
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <codeslayer/codeslayer-utils.h>
#include "snippets-engine.h"
#include "snippets-dialog.h"
#include "snippets-config.h"

static void snippets_engine_class_init  (SnippetsEngineClass *klass);
static void snippets_engine_init        (SnippetsEngine      *engine);
static void snippets_engine_finalize    (SnippetsEngine      *engine);

static gchar* get_config_file_path      (SnippetsEngine      *engine);
static GList* get_configs_deep_copy     (SnippetsEngine      *engine);
static void editor_added_action         (SnippetsEngine       *engine, 
                                         CodeSlayerEditor     *editor);
static gboolean key_press_action        (CodeSlayerEditor     *editor,
                                         GdkEventKey          *event);

#define SNIPPETS_ENGINE_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SNIPPETS_ENGINE_TYPE, SnippetsEnginePrivate))

typedef struct _SnippetsEnginePrivate SnippetsEnginePrivate;

struct _SnippetsEnginePrivate
{
  CodeSlayer *codeslayer;
  GList      *configs;
  gulong      editor_added_id;
};

G_DEFINE_TYPE (SnippetsEngine, snippets_engine, G_TYPE_OBJECT)

static void
snippets_engine_class_init (SnippetsEngineClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = (GObjectFinalizeFunc) snippets_engine_finalize;
  g_type_class_add_private (klass, sizeof (SnippetsEnginePrivate));
}

static void
snippets_engine_init (SnippetsEngine *engine) 
{
  SnippetsEnginePrivate *priv;
  priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);
  priv->configs = NULL;
}

static void
snippets_engine_finalize (SnippetsEngine *engine)
{
  SnippetsEnginePrivate *priv;
  priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);
  if (priv->configs != NULL)
    {
      g_list_foreach (priv->configs, (GFunc) g_object_unref, NULL);
      g_list_free (priv->configs);
      priv->configs = NULL;    
    }

  g_signal_handler_disconnect (priv->codeslayer, priv->editor_added_id);

  G_OBJECT_CLASS (snippets_engine_parent_class)->finalize (G_OBJECT(engine));
}

SnippetsEngine*
snippets_engine_new (CodeSlayer *codeslayer)
{
  SnippetsEnginePrivate *priv;
  SnippetsEngine *engine;

  engine = SNIPPETS_ENGINE (g_object_new (snippets_engine_get_type (), NULL));
  priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);

  priv->codeslayer = codeslayer;
  
  priv->editor_added_id = g_signal_connect_swapped (G_OBJECT (codeslayer), "editor-added",
                                                    G_CALLBACK (editor_added_action), SNIPPETS_ENGINE (engine));
  
  return engine;
}

void
snippets_engine_load_configs (SnippetsEngine *engine)
{
  SnippetsEnginePrivate *priv;
  GList *configs;
  gchar *file_path;

  priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);
  
  file_path = get_config_file_path (engine);
  configs = codeslayer_utils_get_gobjects (SNIPPETS_CONFIG_TYPE,
                                           FALSE,
                                           file_path, 
                                           "snippet",
                                           "file_types", G_TYPE_STRING, 
                                           "name", G_TYPE_STRING, 
                                           "trigger", G_TYPE_STRING, 
                                           "text", G_TYPE_STRING, 
                                            NULL);
  priv->configs = configs;
  g_free (file_path);
}

void
snippets_engine_open_dialog (SnippetsEngine *engine)
{
  SnippetsEnginePrivate *priv;
  GList *copies;
  GtkWidget *dialog;
  gint response;

  priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);

  copies = get_configs_deep_copy (engine);
  dialog = snippets_dialog_new (priv->codeslayer, &copies);
    
  response = gtk_dialog_run (GTK_DIALOG (dialog));
    
  if (response == GTK_RESPONSE_OK)
    {
      gchar *file_path;
      
      g_list_foreach (priv->configs, (GFunc) g_object_unref, NULL);
      g_list_free (priv->configs);      
      priv->configs = copies;
      
      file_path = get_config_file_path (engine);
      
      codeslayer_utils_save_gobjects (copies,
                                      file_path, 
                                      "snippet",
                                      "file_types", G_TYPE_STRING, 
                                      "name", G_TYPE_STRING, 
                                      "trigger", G_TYPE_STRING, 
                                      "text", G_TYPE_STRING, 
                                      NULL);
      g_free (file_path);
    }
  else
    {
      g_list_foreach (copies, (GFunc) g_object_unref, NULL);
      g_list_free (copies);
    }
  
  gtk_widget_destroy (dialog);
}

static gchar*
get_config_file_path (SnippetsEngine *engine)
{
  SnippetsEnginePrivate *priv;
  gchar *folder_path;
  gchar *file_path;
  
  priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);

  folder_path = codeslayer_get_plugins_config_folder_path (priv->codeslayer);  
  file_path = g_build_filename (folder_path, "snippets.xml", NULL);
  g_free (folder_path);
  
  return file_path;
}

static GList*
get_configs_deep_copy (SnippetsEngine *engine)
{
  SnippetsEnginePrivate *priv;
  GList *results = NULL;
  GList *list;

  priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);
  
  list = priv->configs;

  while (list != NULL)
    {
      SnippetsConfig *config = list->data;
      SnippetsConfig *copy;
      const gchar *file_types;
      const gchar *name;
      const gchar *trigger;
      const gchar *text;

      file_types = snippets_config_get_file_types (config);
      name = snippets_config_get_name (config);
      trigger = snippets_config_get_trigger (config);
      text = snippets_config_get_text (config);
      
      copy = snippets_config_new ();
      snippets_config_set_file_types (copy, file_types);
      snippets_config_set_name (copy, name);
      snippets_config_set_trigger (copy, trigger);
      snippets_config_set_text (copy, text);
      results = g_list_prepend (results, copy);
      
      list = g_list_next (list);
    }
    
  return results;    
}

static void 
editor_added_action (SnippetsEngine   *engine, 
                     CodeSlayerEditor *editor)
{  
  g_signal_connect_swapped (G_OBJECT (editor), "key-press-event",
                            G_CALLBACK (key_press_action), editor);  
}

static gboolean
key_press_action (CodeSlayerEditor *editor,
                  GdkEventKey      *event)
{
  g_print ("key press\n");
  
  return FALSE;
}
