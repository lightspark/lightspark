/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010-2012  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010 Alexandre Demers (papouta@hotmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "backends/interfaces/IPlugin.h"

IPlugin::IPlugin ( PLUGIN_TYPES plugin_type, std::string plugin_name, std::string backend_name ) :
		pluginName ( plugin_name ), backendName ( backend_name ), pluginType ( plugin_type )
{

}

const std::string IPlugin::get_pluginName()
{
	return pluginName;
}

PLUGIN_TYPES IPlugin::get_pluginType()
{
	return pluginType;
}

const std::string IPlugin::get_backendName()
{
	return backendName;
}

IPlugin::~IPlugin()
{

}
