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

#ifndef __SNIPPETS_CONFIG_H__
#define	__SNIPPETS_CONFIG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SNIPPETS_CONFIG_TYPE            (snippets_config_get_type ())
#define SNIPPETS_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SNIPPETS_CONFIG_TYPE, SnippetsConfig))
#define SNIPPETS_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SNIPPETS_CONFIG_TYPE, SnippetsConfigClass))
#define IS_SNIPPETS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SNIPPETS_CONFIG_TYPE))
#define IS_SNIPPETS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SNIPPETS_CONFIG_TYPE))

typedef struct _SnippetsConfig SnippetsConfig;
typedef struct _SnippetsConfigClass SnippetsConfigClass;

struct _SnippetsConfig
{
  GObject parent_instance;
};

struct _SnippetsConfigClass
{
  GObjectClass parent_class;
};

GType snippets_config_get_type (void) G_GNUC_CONST;

SnippetsConfig*  snippets_config_new             (void);

const gchar*     snippets_config_get_file_types  (SnippetsConfig *config);
void             snippets_config_set_file_types  (SnippetsConfig *config,
                                                  const gchar    *file_types);
const gchar*     snippets_config_get_name        (SnippetsConfig *config);
void             snippets_config_set_name        (SnippetsConfig *config,
                                                  const gchar    *name);
const gchar*     snippets_config_get_trigger     (SnippetsConfig *config);
void             snippets_config_set_trigger     (SnippetsConfig *config,
                                                  const gchar    *trigger);
const gchar*     snippets_config_get_text        (SnippetsConfig *config);
void             snippets_config_set_text        (SnippetsConfig *config,
                                                  const gchar    *text);

G_END_DECLS

#endif /* __SNIPPETS_CONFIG_H__ */
