/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009,2010  Alessandro Pignotti (a.pignotti@sssup.it)

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

#ifndef AUDIO_H
#define AUDIO_H


#include "compat.h"
#include "decoder.h"
#include <iostream>
#include <boost/filesystem.hpp>

#include "pluginmanager.h"
#include "interfaces/audio/IAudioPlugin.h"

using namespace std;
using namespace boost::filesystem;

//convenience typedef for the pointers to the 2 functions we expect to find in the plugin libraries
typedef IPlugin * ( *PLUGIN_FACTORY ) ();
typedef void ( *PLUGIN_CLEANUP ) ( IPlugin * );

namespace lightspark
{

class AudioManager
{
private:
	vector<string *>audioplugins_list;
	IAudioPlugin *oAudioPlugin;
	string selectedAudioBackend;
	void load_audioplugin ( string selected_backend );
	void release_audioplugin();
	PluginManager *pluginManager;


public:
	AudioManager ( PluginManager *sharePluginManager );
	AudioStream *createStreamPlugin ( AudioDecoder *decoder );
	void freeStreamPlugin ( AudioStream *audioStream );
	bool isTimingAvailablePlugin() const;
	void set_audiobackend ( string desired_backend );
	void get_audioBackendsList();
	void refresh_audioplugins_list();
	~AudioManager();
};

};

#endif
