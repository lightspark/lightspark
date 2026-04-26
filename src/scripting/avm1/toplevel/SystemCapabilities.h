/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef SCRIPTING_AVM1_TOPLEVEL_SYSTEM_CAPABILITIES_H
#define SCRIPTING_AVM1_TOPLEVEL_SYSTEM_CAPABILITIES_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::globals::system_capabilities` crate.

namespace lightspark
{

class AVM1DeclContext;

class AVM1SystemCapabilities : public AVM1Object
{
public:
	AVM1SystemCapabilities(AVM1DeclContext& ctx);

	AVM1_GETTER_DECL(HasAccessibility);
	AVM1_GETTER_DECL(PixelAspectRatio);
	AVM1_GETTER_DECL(ScreenColor);
	AVM1_GETTER_DECL(ScreenDPI);
	AVM1_GETTER_DECL(ScreenResolutionY);
	AVM1_GETTER_DECL(ScreenResolutionX);
	AVM1_GETTER_DECL(HasTLS);
	AVM1_GETTER_DECL(HasVideoEncoder);
	AVM1_GETTER_DECL(HasAudioEncoder);
	AVM1_GETTER_DECL(HasMP3);
	AVM1_GETTER_DECL(HasAudio);
	AVM1_GETTER_DECL(ServerString);
	AVM1_GETTER_DECL(Version);
	AVM1_GETTER_DECL(HasStreamingAudio);
	AVM1_GETTER_DECL(HasStreamingVideo);
	AVM1_GETTER_DECL(HasEmbeddedVideo);
	AVM1_GETTER_DECL(HasPrinting);
	AVM1_GETTER_DECL(HasScreenPlayback);
	AVM1_GETTER_DECL(HasScreenBroadcast);
	AVM1_GETTER_DECL(IsDebugger);
	AVM1_GETTER_DECL(PlayerType);
	AVM1_GETTER_DECL(AvHardwareDisable);
	AVM1_GETTER_DECL(LocalFileReadDisable);
	AVM1_GETTER_DECL(WindowlessDisable);
	AVM1_GETTER_DECL(MaxLevelIDC);
	AVM1_GETTER_DECL(IsEmbeddedInAcrobat);
	AVM1_GETTER_DECL(Manufacturer);
	AVM1_GETTER_DECL(OS);
	AVM1_GETTER_DECL(CpuArchitecture);
	AVM1_GETTER_DECL(Language);
	AVM1_GETTER_DECL(HasIME);
	AVM1_GETTER_DECL(Supports32BitProcesses);
	AVM1_GETTER_DECL(Supports64BitProcesses);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_SYSTEM_CAPABILITIES_H */
