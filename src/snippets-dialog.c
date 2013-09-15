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

#include "snippets-dialog.h"
#include "snippets-config.h"

static void snippets_dialog_class_init  (SnippetsDialogClass *klass);
static void snippets_dialog_init        (SnippetsDialog      *dialog);
static void snippets_dialog_finalize    (SnippetsDialog      *dialog);
                                          
static void registry_changed_action     (SnippetsDialog      *dialog);
static void add_content_area            (SnippetsDialog      *dialog);
static void add_file_types_pane         (SnippetsDialog      *dialog, 
                                         GtkWidget           *hpaned);
static void add_syntax_pane             (SnippetsDialog      *dialog,
                                         GtkWidget           *hpaned);                                         
static void load_configs                (SnippetsDialog      *dialog);                                         
static void tree_add_action             (SnippetsDialog      *dialog);
static void tree_remove_action          (SnippetsDialog      *dialog);
static void tree_edited_action          (SnippetsDialog      *dialog, 
                                         gchar               *tree_path, 
                                         gchar               *file_types);                                         
static void create_popup_menu           (SnippetsDialog      *dialog);
static gboolean show_popup_menu         (SnippetsDialog      *dialog, 
                                         GdkEventButton      *event);
static GList* get_showable_popup_items  (SnippetsDialog      *dialog);
static void add_snippet_action          (SnippetsDialog       *dialog);
static void remove_snippet_action       (SnippetsDialog       *dialog);
static void select_row_action           (GtkTreeSelection     *selection, 
                                         SnippetsDialog       *dialog);                                         
static void trigger_entry_action        (SnippetsDialog       *dialog,
                                         GParamSpec           *spec);                                         
static void text_view_action            (SnippetsDialog       *dialog,
                                         GParamSpec           *spec);                                         
static gint sort_compare                (GtkTreeModel         *model, 
                                         GtkTreeIter          *a,
                                         GtkTreeIter          *b, 
                                         gpointer              userdata);                                         

#define SNIPPETS_DIALOG_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SNIPPETS_DIALOG_TYPE, SnippetsDialogPrivate))

typedef struct _SnippetsDialogPrivate SnippetsDialogPrivate;

struct _SnippetsDialogPrivate
{
  CodeSlayer         *codeslayer;
  CodeSlayerRegistry *registry;
  GtkWidget          *tree;
  GtkTreeStore       *store;
  GList              **configs;
  GtkWidget          *trigger_entry;
  GtkWidget          *text_view;

  gulong              text_buffer_id;
  gulong              trigger_entry_id;

  GtkWidget          *menu;
  GtkWidget          *add_item;
  GtkWidget          *remove_item;
};

enum
{
  TEXT = 0,
  CONFIGURATION,
  COLUMNS
};

G_DEFINE_TYPE (SnippetsDialog, snippets_dialog, GTK_TYPE_DIALOG)

enum
{
  PROP_0,
  PROP_FILE_PATHS
};

static void 
snippets_dialog_class_init (SnippetsDialogClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = (GObjectFinalizeFunc) snippets_dialog_finalize;
  g_type_class_add_private (klass, sizeof (SnippetsDialogPrivate));
}

static void
snippets_dialog_init (SnippetsDialog *dialog)
{
  gtk_window_set_title (GTK_WINDOW (dialog), _("Snippets Configuration"));
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (dialog), TRUE);
}

static void
snippets_dialog_finalize (SnippetsDialog *dialog)
{
  G_OBJECT_CLASS (snippets_dialog_parent_class)-> finalize (G_OBJECT (dialog));
}

GtkWidget*
snippets_dialog_new (CodeSlayer *codeslayer, 
                     GList      **configs)
{
  SnippetsDialogPrivate *priv;
  GtkWidget *dialog;
  
  dialog = g_object_new (snippets_dialog_get_type (), NULL);
  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);

  priv->codeslayer = codeslayer;
  priv->configs = configs;
  priv->registry = codeslayer_get_registry (codeslayer);
  
  add_content_area (SNIPPETS_DIALOG (dialog));
  create_popup_menu (SNIPPETS_DIALOG (dialog));
  
  registry_changed_action (SNIPPETS_DIALOG (dialog));
  
  load_configs (SNIPPETS_DIALOG (dialog));

  return dialog;
}

static void
registry_changed_action (SnippetsDialog *dialog)
{
  SnippetsDialogPrivate *priv;
  
  gdouble editor_tab_width;
  gboolean enable_automatic_indentation;
  gboolean insert_spaces_instead_of_tabs;
  gchar *fontname;
  PangoFontDescription *font_description;
  
  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);
  
  editor_tab_width = codeslayer_registry_get_double (priv->registry,
                                                     CODESLAYER_REGISTRY_EDITOR_TAB_WIDTH);
  gtk_source_view_set_tab_width (GTK_SOURCE_VIEW (priv->text_view), editor_tab_width);
  gtk_source_view_set_indent_width (GTK_SOURCE_VIEW (priv->text_view), -1);

  enable_automatic_indentation = codeslayer_registry_get_boolean (priv->registry,
                                                                     CODESLAYER_REGISTRY_EDITOR_ENABLE_AUTOMATIC_INDENTATION);
  gtk_source_view_set_auto_indent (GTK_SOURCE_VIEW (priv->text_view), 
                                   enable_automatic_indentation);
  gtk_source_view_set_indent_on_tab (GTK_SOURCE_VIEW (priv->text_view),
                                     enable_automatic_indentation);

  insert_spaces_instead_of_tabs = codeslayer_registry_get_boolean (priv->registry,
                                                                      CODESLAYER_REGISTRY_EDITOR_INSERT_SPACES_INSTEAD_OF_TABS);
  gtk_source_view_set_insert_spaces_instead_of_tabs (GTK_SOURCE_VIEW (priv->text_view),
                                                     insert_spaces_instead_of_tabs);

  fontname = codeslayer_registry_get_string (priv->registry,
                                                CODESLAYER_REGISTRY_EDITOR_FONT);
  font_description = pango_font_description_from_string (fontname);
  
  if (fontname)
    g_free (fontname);
  
  gtk_widget_override_font (GTK_WIDGET (priv->text_view), font_description);
  pango_font_description_free (font_description);  
}

static void
add_content_area (SnippetsDialog *dialog)
{
  GtkWidget *content_area;
  GtkWidget *hpaned;

  gtk_dialog_add_buttons (GTK_DIALOG (dialog), 
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
                          
  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  
  hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
                          
  gtk_container_add (GTK_CONTAINER (content_area), hpaned);
  add_file_types_pane (dialog, hpaned);
  add_syntax_pane (dialog, hpaned);
  gtk_widget_show_all (content_area);
}

static void
add_file_types_pane (SnippetsDialog *dialog, 
                     GtkWidget      *hpaned)
{
  SnippetsDialogPrivate *priv;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *tree;
  GtkTreeStore *store;
  GtkTreeSortable *sortable;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;
  GtkWidget *scrolled_window;
  GtkWidget *hbutton;
  GtkWidget *add_button;
  GtkWidget *remove_button;
  
  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  
  label = gtk_label_new ("File Types");
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  /* the tree */

  tree = gtk_tree_view_new ();
  priv->tree = tree;
  
  store = gtk_tree_store_new (COLUMNS, G_TYPE_STRING, G_TYPE_POINTER);
  priv->store = store;
  
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

  sortable = GTK_TREE_SORTABLE (store);
  gtk_tree_sortable_set_sort_func (sortable, TEXT, sort_compare,
                                   GINT_TO_POINTER (TEXT), NULL);
  gtk_tree_sortable_set_sort_column_id (sortable, TEXT, GTK_SORT_ASCENDING);
                           
  column = gtk_tree_view_column_new ();
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "editable", TRUE, "editable-set", TRUE, NULL);

  g_signal_connect_swapped (G_OBJECT (priv->tree), "button_press_event",
                            G_CALLBACK (show_popup_menu), dialog);

  g_signal_connect_swapped (G_OBJECT (renderer), "edited",
                            G_CALLBACK (tree_edited_action), dialog);

  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer, "text", TEXT, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (tree));
  gtk_widget_set_size_request (scrolled_window, -1, 275);

  g_signal_connect (G_OBJECT (selection), "changed",
                    G_CALLBACK (select_row_action), dialog);

  /* the buttons */

  hbutton = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbutton), GTK_BUTTONBOX_START);
  
  add_button = gtk_button_new_from_stock (GTK_STOCK_ADD);
  remove_button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
  
  gtk_box_pack_start (GTK_BOX (hbutton), add_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbutton), remove_button, FALSE, FALSE, 0);
  
  /* pack everything in */  

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbutton, FALSE, FALSE, 0);
  gtk_paned_add1 (GTK_PANED (hpaned), vbox);
  
  g_signal_connect_swapped (G_OBJECT (add_button), "clicked",
                            G_CALLBACK (tree_add_action), dialog);  

  g_signal_connect_swapped (G_OBJECT (remove_button), "clicked",
                            G_CALLBACK (tree_remove_action), dialog);
}

static void
add_syntax_pane (SnippetsDialog *dialog,
                 GtkWidget      *hpaned)
{
  SnippetsDialogPrivate *priv;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *grid;
  GtkWidget *trigger_label;
  GtkWidget *trigger_entry;
  GtkWidget *text_view;
  GtkTextBuffer *buffer;
  GtkWidget *scrolled_window;

  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  label = gtk_label_new (_("Text"));
  gtk_misc_set_alignment (GTK_MISC (label), .02, .5);
  
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  
  /* the start entry */  
  
  trigger_label = gtk_label_new (_("Trigger"));
  trigger_entry =  gtk_entry_new ();
  priv->trigger_entry = trigger_entry;

  gtk_misc_set_alignment (GTK_MISC (trigger_label), 1, .5);
  gtk_misc_set_padding (GTK_MISC (trigger_label), 4, 0);
  gtk_grid_attach (GTK_GRID (grid), trigger_label, 0, 0, 1, 1);
  gtk_grid_attach_next_to (GTK_GRID (grid), trigger_entry, trigger_label, 
                           GTK_POS_RIGHT, 1, 1);
  
  /* the end entry */  
  
  text_view =  gtk_source_view_new ();
  priv->text_view = text_view;
  
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (text_view));
  gtk_widget_set_size_request (scrolled_window, -1, 275);

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 5);
  
  gtk_paned_add2 (GTK_PANED (hpaned), vbox);
    
  priv->trigger_entry_id = g_signal_connect_swapped (G_OBJECT (priv->trigger_entry), "notify::text",
                                                     G_CALLBACK (trigger_entry_action), dialog);
                            
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));

  priv->text_buffer_id = g_signal_connect_swapped (G_OBJECT (buffer), "changed",
                                                 G_CALLBACK (text_view_action), dialog);
}

static void
load_configs (SnippetsDialog *dialog)
{
  SnippetsDialogPrivate *priv;
  GList *list;
  GtkTextBuffer *buffer;
  GtkTreeIter parent;
  const gchar *last_file_types = NULL;

  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);
  
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));

  g_signal_handler_block (priv->trigger_entry, priv->trigger_entry_id);
  g_signal_handler_block (buffer, priv->text_buffer_id);

  list = *priv->configs;
  while (list != NULL)
    {
      SnippetsConfig *config = list->data;
      const gchar *file_types;
      const gchar *name;
      GtkTreeIter iter;
      
      file_types = snippets_config_get_file_types (config);
      
      if (last_file_types == NULL || g_strcmp0 (file_types, last_file_types) != 0)
        {
          gtk_tree_store_append (priv->store, &parent, NULL);
          gtk_tree_store_set (priv->store, &parent, 
                              TEXT, file_types,
                              -1);
                              
          last_file_types = file_types;
        }
        
      name = snippets_config_get_name (config);
      
      gtk_tree_store_append (priv->store, &iter, &parent);
      gtk_tree_store_set (priv->store, &iter, 
                          TEXT, name,
                          CONFIGURATION, config,
                          -1);
                          
      list = g_list_next (list);
    }

  g_signal_handler_unblock (priv->trigger_entry, priv->trigger_entry_id);
  g_signal_handler_unblock (buffer, priv->text_buffer_id);
}

static void
tree_add_action (SnippetsDialog *dialog)
{
  SnippetsDialogPrivate *priv;
  GtkTreeViewColumn *column;
  GtkTreePath *child_path;
  GtkTreeIter iter;

  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);

  gtk_tree_store_append (priv->store, &iter, NULL);
  gtk_tree_store_set (priv->store, &iter, TEXT, "", -1);
  
  column = gtk_tree_view_get_column (GTK_TREE_VIEW (priv->tree), 0);
  child_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), 
                                        &iter);
  gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->tree), child_path, 
                            column, TRUE);
  
  gtk_tree_path_free (child_path);
}

static void                
tree_remove_action (SnippetsDialog *dialog)
{
  SnippetsDialogPrivate *priv;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter parent;

  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
  if (gtk_tree_selection_get_selected (selection, &model, &parent))
    {
      GtkTreeIter iter;
    
      while (gtk_tree_model_iter_children (model, &iter, &parent))
        {
          SnippetsConfig *config;
        
          gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
                              CONFIGURATION, &config, -1);
          
          if (config != NULL)
            {
              *priv->configs = g_list_remove (*priv->configs, config);
              g_object_unref (config);
            }
          
          gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
        }

      gtk_tree_store_remove (GTK_TREE_STORE (model), &parent);
    }
}

static void 
tree_edited_action (SnippetsDialog *dialog, 
                    gchar          *path, 
                    gchar          *text)
{
  SnippetsDialogPrivate *priv;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);
  
  if (!codeslayer_utils_has_text (text))
    return;
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      SnippetsConfig *config;
            
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
                          CONFIGURATION, &config, -1);
            
      gtk_tree_store_set (GTK_TREE_STORE (model), &iter, TEXT, text, -1);        
      
      /* the parent */
      if (config == NULL)
        {
          GtkTreeIter child;
          
          if (!gtk_tree_model_iter_children (model, &child, &iter))
            return;

          do
            {
              SnippetsConfig *config;
              gtk_tree_model_get (GTK_TREE_MODEL (model), &child, 
                                  CONFIGURATION, &config, -1);
              snippets_config_set_file_types (config, text);
            }
          while (gtk_tree_model_iter_next (model, &child));
        }
      else
        {
          snippets_config_set_name (config, text);
        }
    }
}

static void
select_row_action (GtkTreeSelection *selection, 
                   SnippetsDialog   *dialog)
{
  SnippetsDialogPrivate *priv;
  GtkTextBuffer *buffer;
  GtkTreeModel *model;
  GtkTreeIter iter;

  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);
  
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
  
  g_signal_handler_block (priv->trigger_entry, priv->trigger_entry_id);
  g_signal_handler_block (buffer, priv->text_buffer_id);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      SnippetsConfig *config;
      GtkTextBuffer *buffer;
      gboolean toplevel;
    
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
                          CONFIGURATION, &config, -1);
      
      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
      
      toplevel = config == NULL;
        
      if (!toplevel)
        {
          const gchar *text;
          const gchar *trigger; 
          
          text = snippets_config_get_text (config);
          trigger = snippets_config_get_trigger (config);
          
          if (codeslayer_utils_has_text (text))
            gtk_text_buffer_set_text (buffer, text, -1);
          
          if (codeslayer_utils_has_text (trigger))     
            gtk_entry_set_text (GTK_ENTRY (priv->trigger_entry), trigger);
        }
      else
        {
          gtk_text_buffer_set_text (buffer, "", -1);
          gtk_entry_set_text (GTK_ENTRY (priv->trigger_entry), "");
        }
        
      gtk_widget_set_sensitive (GTK_WIDGET (priv->text_view), !toplevel);
      gtk_widget_set_sensitive (GTK_WIDGET (priv->trigger_entry), !toplevel);
    }
    
  g_signal_handler_unblock (priv->trigger_entry, priv->trigger_entry_id);
  g_signal_handler_unblock (buffer, priv->text_buffer_id);
}

static void
trigger_entry_action (SnippetsDialog *dialog,
                    GParamSpec     *spec)
{
  SnippetsDialogPrivate *priv;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      SnippetsConfig *config;
    
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
                          CONFIGURATION, &config, -1);
      
      if (config != NULL)
        {
          const gchar *text;
          text = gtk_entry_get_text (GTK_ENTRY (priv->trigger_entry));
          snippets_config_set_trigger (config, text);
        }    
    }
}

static void
text_view_action (SnippetsDialog *dialog,
                  GParamSpec     *spec)
{
  SnippetsDialogPrivate *priv;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  
  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);
  
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      SnippetsConfig *config;
    
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
                          CONFIGURATION, &config, -1);
      
      if (config != NULL)
        {
          GtkTextBuffer *buffer;
          GtkTextIter start;
          GtkTextIter end;
          gchar *text;
          
          buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
          gtk_text_buffer_get_bounds (buffer, &start, &end);
          text = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
          
          snippets_config_set_text (config, text);
          
          g_free (text);
        }
    }
}

static void
add_snippet_action (SnippetsDialog *dialog)
{
  SnippetsDialogPrivate *priv;
  GtkTreeSelection *tree_selection;
  GtkTreeModel *model;
  GtkTreeIter parent;
  
  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);
  
  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
  if (gtk_tree_selection_get_selected (tree_selection, &model, &parent))
    {
      GtkTreeIter iter;
      GtkTreeViewColumn *column;
      GtkTreePath *child_path;
      GtkTreePath *tree_path;
      gchar *file_types;
      SnippetsConfig *config;

      config = snippets_config_new ();
      snippets_config_set_name (config, "");
      snippets_config_set_trigger (config, "");
      snippets_config_set_text (config, "");
      
      gtk_tree_model_get (GTK_TREE_MODEL (model), &parent, 
                          TEXT, &file_types, -1);
                          
      snippets_config_set_file_types (config, file_types);
      g_free (file_types);

      *priv->configs = g_list_append (*priv->configs, config);
      
      gtk_tree_store_append (priv->store, &iter, &parent);
      gtk_tree_store_set (priv->store, &iter, 
                          TEXT, "",
                          CONFIGURATION, config, -1);
                          
      tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &parent);
      gtk_tree_view_expand_row (GTK_TREE_VIEW (priv->tree), tree_path, FALSE);

      column = gtk_tree_view_get_column (GTK_TREE_VIEW (priv->tree), 0);
      child_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &iter);
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (priv->tree), child_path, column, TRUE);
  
      gtk_tree_path_free (tree_path);
      gtk_tree_path_free (child_path);      
    }
}

static void
remove_snippet_action (SnippetsDialog *dialog)
{
  SnippetsDialogPrivate *priv;
  GtkTreeSelection *tree_selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  
  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);
  
  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
  if (gtk_tree_selection_get_selected (tree_selection, &model, &iter))
    {
      SnippetsConfig *config;
    
      gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
                          CONFIGURATION, &config, -1);
      
      *priv->configs = g_list_remove (*priv->configs, config);
      g_object_unref (config);
      
      gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
    }
}

static void
create_popup_menu (SnippetsDialog *dialog)
{
  SnippetsDialogPrivate *priv;
  
  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);

  priv->menu = gtk_menu_new ();

  priv->add_item = gtk_menu_item_new_with_label (_("Add Snippet"));
  g_signal_connect_swapped (G_OBJECT (priv->add_item), "activate",
                            G_CALLBACK (add_snippet_action), dialog);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), priv->add_item);

  priv->remove_item = gtk_menu_item_new_with_label (_("Remove Snippet"));
  g_signal_connect_swapped (G_OBJECT (priv->remove_item), "activate",
                            G_CALLBACK (remove_snippet_action), dialog);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), priv->remove_item);
}

static gboolean
show_popup_menu (SnippetsDialog *dialog, 
                 GdkEventButton *event)
{
  SnippetsDialogPrivate *priv;
  
  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);

  if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
      GtkTreeSelection *tree_selection;
      GList *items;
      GList *tmp;

      gtk_container_foreach (GTK_CONTAINER (priv->menu), (GtkCallback) gtk_widget_hide, NULL);

      tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
      if (gtk_tree_selection_count_selected_rows (tree_selection) <= 1)
        {
          GtkTreePath *path;

          /* Get tree path for row that was clicked */
          if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (priv->tree),
                                            (gint) event->x, 
                                            (gint) event->y,
                                            &path, NULL, NULL, NULL))
            {
              gtk_tree_selection_unselect_all (tree_selection);
              gtk_tree_selection_select_path (tree_selection, path);
              gtk_tree_path_free (path);
            }
        }

      items = get_showable_popup_items (dialog);
      tmp = items;
      
      if (tmp != NULL)
        {
          while (tmp != NULL)
            {
              GtkWidget *popup_menu_item = tmp->data;
              gtk_widget_show_all (popup_menu_item);
              tmp = g_list_next (tmp);
            }
          g_list_free (items);
          
          gtk_menu_popup (GTK_MENU (priv->menu), NULL, NULL, NULL, NULL, 
                          (event != NULL) ? event->button : 0,
                          gdk_event_get_time ((GdkEvent *) event));

          return TRUE;
        }
    }

  return FALSE;
}

static GList*
get_showable_popup_items (SnippetsDialog *dialog)
{
  SnippetsDialogPrivate *priv;
    
  GtkTreeSelection *tree_selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  GList *results = NULL;

  priv = SNIPPETS_DIALOG_GET_PRIVATE (dialog);

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));

  if (gtk_tree_selection_get_selected (tree_selection, &model, &iter))
    {
      GtkTreePath *tree_path;
      gint depth;
      
      tree_path = gtk_tree_model_get_path (model, &iter);
      depth = gtk_tree_path_get_depth (tree_path);
      
      if (depth == 1)
        results = g_list_append (results, priv->add_item);
      else if (depth == 2)
        results = g_list_append (results, priv->remove_item);
        
      gtk_tree_path_free (tree_path);
    }
    
  return results;
}                 

static gint
sort_compare (GtkTreeModel *model, 
              GtkTreeIter  *a,
              GtkTreeIter  *b, 
              gpointer      userdata)
{
  gint sortcol;
  gint ret = 0;

  sortcol = GPOINTER_TO_INT (userdata);
  
  switch (sortcol)
    {
    case TEXT:
      {
        gchar *text1, *text2;

        gtk_tree_model_get (model, a, TEXT, &text1, -1);
        gtk_tree_model_get (model, b, TEXT, &text2, -1);

        ret = g_strcmp0 (text1, text2);

        g_free (text1);
        g_free (text2);
      }
      break;
    }

  return ret;
}
