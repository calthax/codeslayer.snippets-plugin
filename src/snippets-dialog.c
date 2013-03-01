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

static void snippets_dialog_class_init  (SnippetsDialogClass  *klass);
static void snippets_dialog_init        (SnippetsDialog       *dialog);
static void snippets_dialog_finalize    (SnippetsDialog       *dialog);
                                          
static void add_content_area            (SnippetsDialog       *dialog);

#define SNIPPETS_DIALOG_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SNIPPETS_DIALOG_TYPE, SnippetsDialogPrivate))

typedef struct _SnippetsDialogPrivate SnippetsDialogPrivate;

struct _SnippetsDialogPrivate
{
  CodeSlayer   *codeslayer;
  GtkWidget    *tree;
  GtkListStore *store;
  GList        **configs;
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
  gtk_window_set_title (GTK_WINDOW (dialog), "Snippets Configuration");
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
  
  add_content_area (SNIPPETS_DIALOG (dialog));
  
  return dialog;
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
  gtk_widget_show_all (content_area);
}
