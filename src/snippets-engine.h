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

#ifndef __SNIPPETS_ENGINE_H__
#define	__SNIPPETS_ENGINE_H__

#include <gtk/gtk.h>
#include <codeslayer/codeslayer.h>

G_BEGIN_DECLS

#define SNIPPETS_ENGINE_TYPE            (snippets_engine_get_type ())
#define SNIPPETS_ENGINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SNIPPETS_ENGINE_TYPE, SnippetsEngine))
#define SNIPPETS_ENGINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SNIPPETS_ENGINE_TYPE, SnippetsEngineClass))
#define IS_SNIPPETS_ENGINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SNIPPETS_ENGINE_TYPE))
#define IS_SNIPPETS_ENGINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SNIPPETS_ENGINE_TYPE))

typedef struct _SnippetsEngine SnippetsEngine;
typedef struct _SnippetsEngineClass SnippetsEngineClass;

struct _SnippetsEngine
{
  GObject parent_instance;
};

struct _SnippetsEngineClass
{
  GObjectClass parent_class;
};

GType snippets_engine_get_type (void) G_GNUC_CONST;

SnippetsEngine*  snippets_engine_new           (CodeSlayer     *codeslayer);
                                        
void             snippets_engine_load_configs  (SnippetsEngine *engine);

void             snippets_engine_open_dialog   (SnippetsEngine *engine);

G_END_DECLS

#endif /* _SNIPPETS_ENGINE_H */
