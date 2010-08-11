/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)
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


#ifndef IPLUGIN_H
#define IPLUGIN_H

#include <iostream>
#include "../../compat.h"

using namespace std;

enum PLUGIN_TYPES { UNDEFINED=0, AUDIO };

class IPlugin
{
  public:
    IPlugin(PLUGIN_TYPES init_Type = UNDEFINED, string init_Name = "undefined");
    virtual const string get_pluginName() = 0;
    virtual PLUGIN_TYPES get_pluginType() = 0;
    virtual ~IPlugin() = 0;
  protected:
    PLUGIN_TYPES pluginType; //type of plugin of PLUGIN_TYPES
    string pluginName; //name of the plugin
};

/*************************
Extern "C" functions that each plugin must implement in order to be recognized as a plugin by us.
It allows us to share a common interface between plugins and the application.
 
Plugin factory function
extern "C" IPlugin* Create_Plugin();

Plugin cleanup function
extern "C" void Release_Plugin(IPlugin* p_plugin);
***************************/

#endif