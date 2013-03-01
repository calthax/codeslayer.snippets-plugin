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

#ifndef __SNIPPETS_DIALOG_H__
#define	__SNIPPETS_DIALOG_H__

#include <gtk/gtk.h>
#include <codeslayer/codeslayer.h>
#include <codeslayer/codeslayer-utils.h>

G_BEGIN_DECLS

#define SNIPPETS_DIALOG_TYPE            (snippets_dialog_get_type ())
#define SNIPPETS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SNIPPETS_DIALOG_TYPE, SnippetsDialog))
#define SNIPPETS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SNIPPETS_DIALOG_TYPE, SnippetsDialogClass))
#define IS_SNIPPETS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SNIPPETS_DIALOG_TYPE))
#define IS_SNIPPETS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SNIPPETS_DIALOG_TYPE))

typedef struct _SnippetsDialog SnippetsDialog;
typedef struct _SnippetsDialogClass SnippetsDialogClass;

struct _SnippetsDialog
{
  GtkDialog parent_instance;
};

struct _SnippetsDialogClass
{
  GtkDialogClass parent_class;
};

GType snippets_dialog_get_type (void) G_GNUC_CONST;
     
GtkWidget*  snippets_dialog_new  (CodeSlayer *codeslayer, 
                                  GList      **configurations);

G_END_DECLS

#endif /* __SNIPPETS_DIALOG_H__ */
