/*
 *  profilesx
 *  Copyright (C) 2010 Nicolai Hess
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef PROFILESX_STATUS_PANEL_PLUGIN_H
#define PROFILESX_STATUS_PANEL_PLUGIN_H

#include <libhildondesktop/libhildondesktop.h>

G_BEGIN_DECLS

#define TYPE_PROFILESX_STATUS_PLUGIN (profilesx_status_plugin_get_type())

#define PROFILESX_STATUS_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),	\
								 TYPE_PROFILESX_STATUS_PLUGIN, ProfilesxStatusPlugin))

#define PROFILESX_STATUS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klas), \
								      TYPE_PROFILESX_STATUS_PLUGIN, ProfilesxStatusPluginClass))

#define IS_PROFILESX_STATUS_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
								    TYPE_PROFILESX_STATUS_PLUGIN))

#define IS_PROFILES_STATUS_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
									TYPE_PROFILESX_STATUS_PLUGIN))

#define PROFILESX_STATUS_PLUGIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
									  TYPE_PROFILESX_STATUS_PLUGIN, ProfilesxStatusPluginClass))

#define STATUS_AREA_EXAMPLE_ICON_SIZE 22

typedef struct _ProfilesxStatusPlugin ProfilesxStatusPlugin;
typedef struct _ProfilesxStatusPluginClass ProfilesxStatusPluginClass;
typedef struct _ProfilesxStatusPluginPrivate ProfilesxStatusPluginPrivate;

struct _ProfilesxStatusPlugin
{
  HDStatusMenuItem parent;
  ProfilesxStatusPluginPrivate *priv;
};

struct _ProfilesxStatusPluginClass
{
  HDStatusMenuItemClass parent;
};

GType profilesx_status_plugin_get_type(void);

G_END_DECLS
#endif
