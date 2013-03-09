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

#include <gdk/gdkkeysyms.h>
#include <codeslayer/codeslayer-utils.h>
#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "snippets-engine.h"
#include "snippets-dialog.h"
#include "snippets-config.h"

static void snippets_engine_class_init  (SnippetsEngineClass *klass);
static void snippets_engine_init        (SnippetsEngine      *engine);
static void snippets_engine_finalize    (SnippetsEngine      *engine);

static void save_configs                (SnippetsEngine      *engine);
static void load_configs                (xmlNode             *a_node,
                                         GList               **configs);
static gchar* get_config_file_path      (SnippetsEngine      *engine);
static GList* get_configs_deep_copy     (SnippetsEngine      *engine);
static void editor_added_action         (SnippetsEngine       *engine, 
                                         CodeSlayerEditor     *editor);
static gboolean key_press_action        (CodeSlayerEditor     *editor,
                                         GdkEventKey          *event, 
                                         SnippetsEngine       *engine);
static void move_iter_word_start        (GtkTextIter          *iter);

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
  GList *editors;
  GList *tmp;

  engine = SNIPPETS_ENGINE (g_object_new (snippets_engine_get_type (), NULL));
  priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);

  priv->codeslayer = codeslayer;
  
  editors = codeslayer_get_all_editors (codeslayer);
  
  tmp = editors;
  
  while (tmp != NULL)
    {
      CodeSlayerEditor *editor = tmp->data;
      editor_added_action (SNIPPETS_ENGINE (engine), editor);
      tmp = g_list_next (tmp);
    }
    
  g_list_free (editors);
  
  priv->editor_added_id = g_signal_connect_swapped (G_OBJECT (codeslayer), "editor-added",
                                                    G_CALLBACK (editor_added_action), SNIPPETS_ENGINE (engine));

  return engine;
}

void
snippets_engine_load_configs (SnippetsEngine *engine)
{
  SnippetsEnginePrivate *priv;
  gchar *file_path;
  xmlDoc *doc = NULL;
  xmlNode *root_element = NULL;
  
  priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);

  file_path = get_config_file_path (engine);
  if (file_path == NULL) 
    return;

  doc = xmlReadFile (file_path, NULL, 0);

  if (doc == NULL) 
    {
      g_warning ("could not parse snippets file %s\n", file_path);
      xmlCleanupParser();
      return;
    }

  root_element = xmlDocGetRootElement (doc);

  load_configs (root_element, &priv->configs);

  xmlFreeDoc (doc);
  xmlCleanupParser ();
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
      g_list_foreach (priv->configs, (GFunc) g_object_unref, NULL);
      g_list_free (priv->configs);      
      priv->configs = copies;
      
      save_configs (engine);
    }
  else
    {
      g_list_foreach (copies, (GFunc) g_object_unref, NULL);
      g_list_free (copies);
    }
  
  gtk_widget_destroy (dialog);
}

static void
load_configs (xmlNode *a_node, 
              GList   **configs)
{
  xmlNode *cur_node = NULL;

  for (cur_node = a_node; cur_node; cur_node = cur_node->next) 
    {
      if (cur_node->type == XML_ELEMENT_NODE)
        {
          if (g_strcmp0 ((gchar*)cur_node->name, "snippet") == 0)
            {
              SnippetsConfig *config;
              xmlChar *file_types;
              xmlChar *name;
              xmlChar *text;
              xmlChar *trigger;
              
              config = snippets_config_new ();
            
              file_types = xmlGetProp (cur_node, (const xmlChar*)"file_types");
              name = xmlGetProp (cur_node, (const xmlChar*)"name");
              trigger = xmlGetProp (cur_node, (const xmlChar*)"trigger");
              text = xmlNodeGetContent (cur_node);
              
              snippets_config_set_file_types (config, (gchar*)file_types);
              snippets_config_set_name (config, (gchar*)name);
              snippets_config_set_text (config, (gchar*)text);
              snippets_config_set_trigger (config, (gchar*)trigger);
              
              xmlFree (file_types);
              xmlFree (name);
              xmlFree (trigger);
              xmlFree (text);
              
              *configs = g_list_append (*configs, config);
            }
        }
      load_configs (cur_node->children, configs);
    }
}

static void
save_configs (SnippetsEngine *engine)
{
  SnippetsEnginePrivate *priv;
  GList *list;
  xmlDocPtr doc;
  xmlNodePtr root_node;
  gchar *file_path;
  
  priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);
  
  file_path = get_config_file_path (engine);
  
  doc = xmlNewDoc (BAD_CAST "1.0");
  
  root_node = xmlNewNode (NULL, BAD_CAST "snippets");
  xmlDocSetRootElement (doc, root_node);
  
  list = priv->configs;
  while (list != NULL)
    {
      SnippetsConfig *config = list->data;
      xmlNodePtr node, cdata;
      const gchar *text;
      
      node = xmlNewChild (root_node, NULL, BAD_CAST "snippet", NULL);
      xmlNewProp(node, BAD_CAST "file_types", BAD_CAST snippets_config_get_file_types (config));
      xmlNewProp(node, BAD_CAST "name", BAD_CAST snippets_config_get_name (config));
      xmlNewProp(node, BAD_CAST "trigger", BAD_CAST snippets_config_get_trigger (config));
      
      text = snippets_config_get_text (config);
      cdata = xmlNewCDataBlock (doc, (const xmlChar*)text, g_utf8_strlen (text, -1));
      xmlAddChild (node, cdata);      

      list = g_list_next (list);
    }
  
  xmlSaveFormatFileEnc(file_path, doc, "UTF-8", 1);

  xmlFreeDoc(doc);
  xmlCleanupParser();
  g_free (file_path);
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
  g_signal_connect (G_OBJECT (editor), "key-press-event",
                    G_CALLBACK (key_press_action), engine);  
}

static gboolean
key_press_action (CodeSlayerEditor *editor,
                  GdkEventKey      *event, 
                  SnippetsEngine   *engine)
{
  if (event->keyval == GDK_KEY_Tab)
    {
      SnippetsEnginePrivate *priv;
      CodeSlayerDocument *document;
      const gchar *file_path;
      GtkTextBuffer *buffer;
      GtkTextMark *insert_mark;
      GtkTextIter iter;
      GtkTextIter start;
      gchar *word;
      GList *list;
      
      priv = SNIPPETS_ENGINE_GET_PRIVATE (engine);

      document = codeslayer_get_active_editor_document (priv->codeslayer);
      file_path = codeslayer_document_get_file_path (document);

      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (editor));
      insert_mark = gtk_text_buffer_get_insert (buffer);
      
      gtk_text_buffer_get_iter_at_mark (buffer, &iter, insert_mark);
      
      start = iter;
      move_iter_word_start (&start);
      
      word = gtk_text_iter_get_text (&start, &iter);
      
      if (!codeslayer_utils_has_text (word))
        return FALSE;

      list = priv->configs;
      while (list != NULL)
        {
          SnippetsConfig *config = list->data;
          const gchar *file_types;
          const gchar *trigger;
          GList *elements;
          
          file_types = snippets_config_get_file_types (config);
          
          elements = codeslayer_utils_string_to_list (file_types);
          
          if (!codeslayer_utils_contains_element_with_suffix (elements, file_path))
            {
              g_list_foreach (elements, (GFunc) g_free, NULL);
              g_list_free (elements);
              list = g_list_next (list);
              continue;
            }
          
          trigger = snippets_config_get_trigger (config);
          if (g_strcmp0 (word, trigger) == 0)
            {
              const gchar *text;
              text = snippets_config_get_text (config);

              gtk_text_buffer_begin_user_action (buffer);
              gtk_text_buffer_delete (buffer, &start, &iter);
              gtk_text_buffer_insert (buffer, &start, text, -1);
              gtk_text_buffer_end_user_action (buffer);
              return TRUE;
            }

          list = g_list_next (list);
        }
    }
  
  return FALSE;
}

static void
move_iter_word_start (GtkTextIter *iter)
{
  gunichar ctext;
  gtk_text_iter_backward_char (iter);
  ctext = gtk_text_iter_get_char (iter);
  
  while (g_ascii_isalnum (ctext) || ctext == '_')
    {
      gtk_text_iter_backward_char (iter);
      ctext = gtk_text_iter_get_char (iter);
    }
    
  gtk_text_iter_forward_char (iter);    
}
