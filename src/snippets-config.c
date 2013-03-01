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

#include "snippets-config.h"

static void snippets_config_class_init    (SnippetsConfigClass *klass);
static void snippets_config_init          (SnippetsConfig      *config);
static void snippets_config_finalize      (SnippetsConfig      *config);
static void snippets_config_get_property  (GObject             *object, 
                                           guint                prop_id,
                                           GValue              *value,
                                           GParamSpec          *pspec);
static void snippets_config_set_property  (GObject             *object, 
                                           guint                prop_id,
                                           const GValue        *value,
                                           GParamSpec          *pspec);

#define SNIPPETS_CONFIG_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), SNIPPETS_CONFIG_TYPE, SnippetsConfigPrivate))

typedef struct _SnippetsConfigPrivate SnippetsConfigPrivate;

struct _SnippetsConfigPrivate
{
  gchar *file_types;
  gchar *name;
  gchar *trigger;
  gchar *text;
};

enum
{
  PROP_0,
  PROP_FILE_TYPES,
  PROP_NAME,
  PROP_TRIGGER,
  PROP_TEXT
};

G_DEFINE_TYPE (SnippetsConfig, snippets_config, G_TYPE_OBJECT)
     
static void 
snippets_config_class_init (SnippetsConfigClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = (GObjectFinalizeFunc) snippets_config_finalize;

  gobject_class->get_property = snippets_config_get_property;
  gobject_class->set_property = snippets_config_set_property;

  g_type_class_add_private (klass, sizeof (SnippetsConfigPrivate));

  g_object_class_install_property (gobject_class, 
                                   PROP_FILE_TYPES,
                                   g_param_spec_string ("file_types", 
                                                        "File Types",
                                                        "File Types", 
                                                        "",
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, 
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "Name",
                                                        "",
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, 
                                   PROP_TRIGGER,
                                   g_param_spec_string ("trigger",
                                                        "Trigger",
                                                        "Trigger",
                                                        "",
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, 
                                   PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        "Text",
                                                        "Text",
                                                        "",
                                                        G_PARAM_READWRITE));
}

static void
snippets_config_init (SnippetsConfig *config)
{
  SnippetsConfigPrivate *priv;
  priv = SNIPPETS_CONFIG_GET_PRIVATE (config);
  priv->file_types = NULL;
  priv->name = NULL;
  priv->trigger = NULL;
}

static void
snippets_config_finalize (SnippetsConfig *config)
{
  SnippetsConfigPrivate *priv;
  priv = SNIPPETS_CONFIG_GET_PRIVATE (config);
  if (priv->file_types)
    {
      g_free (priv->file_types);
      priv->file_types = NULL;
    }
  if (priv->name)
    {
      g_free (priv->name);
      priv->name = NULL;
    }
  if (priv->trigger)
    {
      g_free (priv->trigger);
      priv->trigger = NULL;
    }
  if (priv->text)
    {
      g_free (priv->text);
      priv->text = NULL;
    }
  G_OBJECT_CLASS (snippets_config_parent_class)->finalize (G_OBJECT (config));
}

static void
snippets_config_get_property (GObject    *object, 
                              guint       prop_id,
                              GValue     *value, 
                              GParamSpec *pspec)
{
  SnippetsConfig *config;
  SnippetsConfigPrivate *priv;
  
  config = SNIPPETS_CONFIG (object);
  priv = SNIPPETS_CONFIG_GET_PRIVATE (config);

  switch (prop_id)
    {
    case PROP_FILE_TYPES:
      g_value_set_string (value, priv->file_types);
      break;
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_TRIGGER:
      g_value_set_string (value, priv->trigger);
      break;
    case PROP_TEXT:
      g_value_set_string (value, priv->text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
snippets_config_set_property (GObject      *object, 
                              guint         prop_id,
                              const GValue *value, 
                              GParamSpec   *pspec)
{
  SnippetsConfig *config;
  config = SNIPPETS_CONFIG (object);

  switch (prop_id)
    {
    case PROP_FILE_TYPES:
      snippets_config_set_file_types (config, g_value_get_string (value));
      break;
    case PROP_NAME:
      snippets_config_set_name (config, g_value_get_string (value));
      break;
    case PROP_TRIGGER:
      snippets_config_set_trigger (config, g_value_get_string (value));
      break;
    case PROP_TEXT:
      snippets_config_set_text (config, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

SnippetsConfig*
snippets_config_new (void)
{
  return SNIPPETS_CONFIG (g_object_new (snippets_config_get_type (), NULL));
}

const gchar*
snippets_config_get_file_types (SnippetsConfig *config)
{
  return SNIPPETS_CONFIG_GET_PRIVATE (config)->file_types;
}

void
snippets_config_set_file_types (SnippetsConfig *config, 
                                const gchar    *file_types)
{
  SnippetsConfigPrivate *priv;
  priv = SNIPPETS_CONFIG_GET_PRIVATE (config);
  if (priv->file_types)
    {
      g_free (priv->file_types);
      priv->file_types = NULL;
    }
  priv->file_types = g_strdup (file_types);
}

const gchar*
snippets_config_get_name (SnippetsConfig *config)
{
  return SNIPPETS_CONFIG_GET_PRIVATE (config)->name;
}

void
snippets_config_set_name (SnippetsConfig *config,
                           const gchar    *name)
{
  SnippetsConfigPrivate *priv;
  priv = SNIPPETS_CONFIG_GET_PRIVATE (config);
  if (priv->name)
    {
      g_free (priv->name);
      priv->name = NULL;
    }
  priv->name = g_strdup (name);
}

const gchar*
snippets_config_get_trigger (SnippetsConfig *config)
{
  return SNIPPETS_CONFIG_GET_PRIVATE (config)->trigger;
}

void
snippets_config_set_trigger (SnippetsConfig *config,
                         const gchar    *trigger)
{
  SnippetsConfigPrivate *priv;
  priv = SNIPPETS_CONFIG_GET_PRIVATE (config);
  if (priv->trigger)
    {
      g_free (priv->trigger);
      priv->trigger = NULL;
    }
  priv->trigger = g_strdup (trigger);
}

const gchar*
snippets_config_get_text (SnippetsConfig *config)
{
  return SNIPPETS_CONFIG_GET_PRIVATE (config)->text;
}

void
snippets_config_set_text (SnippetsConfig *config,
                          const gchar    *text)
{
  SnippetsConfigPrivate *priv;
  priv = SNIPPETS_CONFIG_GET_PRIVATE (config);
  if (priv->text)
    {
      g_free (priv->text);
      priv->text = NULL;
    }
  priv->text = g_strdup (text);
}
