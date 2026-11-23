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

#include <initializer_list>

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/SystemCapabilities.h"
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::system_capabilities` crate.

using SysCaps = SystemCapabilities;
using AVM1SysCaps = AVM1SystemCapabilities;

using AVM1SystemCapabilities;

static constexpr auto objDecls =
{
	AVM1_GETTER_PROTO(hasAccessibility, HasAccessibility),
	AVM1_GETTER_PROTO(pixelAspectRatio, PixelAspectRatio),
	AVM1_GETTER_PROTO(screenColor, ScreenColor),
	AVM1_GETTER_PROTO(screenDPI, ScreenDPI),
	AVM1_GETTER_PROTO(screenResolutionY, ScreenResolutionY),
	AVM1_GETTER_PROTO(screenResolutionX, ScreenResolutionX),
	AVM1_GETTER_PROTO(hasTLS, HasTLS),
	AVM1_GETTER_PROTO(hasVideoEncoder, HasVideoEncoder),
	AVM1_GETTER_PROTO(hasAudioEncoder, HasAudioEncoder),
	AVM1_GETTER_PROTO(hasMP3, HasMP3),
	AVM1_GETTER_PROTO(hasAudio, HasAudio),
	AVM1_GETTER_PROTO(serverString, ServerString),
	AVM1_GETTER_PROTO(version, Version),
	AVM1_GETTER_PROTO(hasStreamingAudio, HasStreamingAudio),
	AVM1_GETTER_PROTO(hasStreamingVideo, HasStreamingVideo),
	AVM1_GETTER_PROTO(hasEmbeddedVideo, HasEmbeddedVideo),
	AVM1_GETTER_PROTO(hasPrinting, HasPrinting),
	AVM1_GETTER_PROTO(hasScreenPlayback, HasScreenPlayback),
	AVM1_GETTER_PROTO(hasScreenBroadcast, HasScreenBroadcast),
	AVM1_GETTER_PROTO(isDebugger, IsDebugger),
	AVM1_GETTER_PROTO(playerType, PlayerType),
	AVM1_GETTER_PROTO(avHardwareDisable, AvHardwareDisable),
	AVM1_GETTER_PROTO(localFileReadDisable, LocalFileReadDisable),
	AVM1_GETTER_PROTO(windowlessDisable, WindowlessDisable),
	AVM1_GETTER_PROTO(maxLevelIDC, MaxLevelIDC),
	AVM1_GETTER_PROTO(isEmbeddedInAcrobat, IsEmbeddedInAcrobat),
	AVM1_GETTER_PROTO(manufacturer, Manufacturer),
	AVM1_GETTER_PROTO(os, OS),
	AVM1_GETTER_PROTO(cpuArchitecture, CpuArchitecture),
	AVM1_GETTER_PROTO(language, Language),
	AVM1_GETTER_PROTO(hasIME, HasIME),
	AVM1_GETTER_PROTO(supports32BitProcesses, Supports32BitProcesses),
	AVM1_GETTER_PROTO(supports64BitProcesses, Supports64BitProcesses)
};

AVM1SysCaps::AVM1SystemCapabilities(AVM1DeclContext& ctx) : AVM1Object
(
	ctx.getGcCtx(),
	ctx.getObjectProto()
)
{
	ctx.definePropsOn(this, objDecls);
}

static bool hasCap(AVM1Activation& act, const SysCaps& cap)
{
	return act.getSys()->getEngineData()->hasCapability(cap);
}

#define CAPAPILITIES_FUNC(name, cap) AVM1_GETTER_BODY(AVM1SysCaps, name) \
{ \
	return hasCap(act, SysCaps::cap); \
}

#define INV_CAPAPILITIES_FUNC(name, cap) AVM1_GETTER_BODY(AVM1SysCaps, name) \
{ \
	return !hasCap(act, SysCaps::cap); \
}

CAPABILITIES_FUNC(HasAccessibility, Accessibility);

AVM1_GETTER_BODY(AVM1SysCaps, PixelAspectRatio)
{
	return act.getSys()->getEngineData()->getPixelAspectRatio();
}

AVM1_GETTER_BODY(AVM1SysCaps, ScreenColor)
{
	std::stringstream s;
	auto engineData = act.getSys()->getEngineData();
	return (s << engineData->getScreenColor()).str();
}

AVM1_GETTER_BODY(AVM1SysCaps, ScreenDPI)
{
	auto engineData = act.getSys()->getEngineData();
	return engineData->getScreenDPI();
}

AVM1_GETTER_BODY(AVM1SysCaps, ScreenResolutionY)
{
	auto engineData = act.getSys()->getEngineData();
	return engineData->getScreenSize().y;
}

AVM1_GETTER_BODY(AVM1SysCaps, ScreenResolutionX)
{
	auto engineData = act.getSys()->getEngineData();
	return engineData->getScreenSize().x;
}

CAPABILITIES_FUNC(HasTLS, TLS);
CAPABILITIES_FUNC(HasVideoEncoder, VideoEncoder);
CAPABILITIES_FUNC(HasAudioEncoder, AudioEncoder);
CAPABILITIES_FUNC(HasMP3, MP3);
CAPABILITIES_FUNC(HasAudio, Audio);

AVM1_GETTER_BODY(AVM1SysCaps, ServerString)
{
	auto encodeCap = [&](const SysCaps& cap)
	{
		return hasCap(act, cap) ? 't' : 'f';
	};

	auto encodeInvCap = [&](const SysCaps& cap)
	{
		return hasCap(act, cap) ? 'f' : 't';
	};

	auto encodeStr = [](const tiny_string& str)
	{
		return URLInfo::encode(str, URLInfo::ENCODE_FORM);
	};

	auto engineData = act.getSys()->getEngineData();
	auto screenInfo = engineData->getScreenInfo();

	std::stringstream s;
	return
	(
		s << "A=" << encodeCap(SysCaps::Audio) <<
		"&SA=" << encodeCap(SysCaps::StreamingAudio) <<
		"&SV=" << encodeCap(SysCaps::StreamingVideo) <<
		"&EV=" << encodeCap(SysCaps::EmbeddedVideo) <<
		"&MP3=" << encodeCap(SysCaps::MP3) <<
		"&AE=" << encodeCap(SysCaps::AudioEncoder) <<
		"&VE=" << encodeCap(SysCaps::VideoEncoder) <<
		"&ACC=" << encodeCap(SysCaps::Accessability) <<
		"&PR=" << encodeCap(SysCaps::Printing) <<
		"&SP=" << encodeCap(SysCaps::ScreenPlayback) <<
		"&SB=" << encodeCap(SysCaps::ScreenBroadcast) <<
		"&DEB=" << encodeCap(SysCaps::Debugger) <<
		"&V=" << encodeStr(sys->getVersionString()) <<
		"&M=" << encodeStr(engineData->getManufacture()) <<
		"&R=" << screenInfo.size.x << 'x' << screenInfo.size.y <<
		"&DP=" << screenInfo.dpi <<
		"&COL=" << screenInfo.getColorStr() <<
		"&AR=" << engineData->getPixelAspectRatio() <<
		"&OS=" << encodeStr(engineData->platformOS) <<
		"&L=" << engineData->getLangCode(act.getSys()) <<
		"&IME=" << encodeCap(SysCaps::IME) <<
		"&PT=" << engineData->getPlayerType() <<
		"&AVD=" << encodeInvCap(SysCaps::AvHardware) <<
		"&LFD=" << encodeInvCap(SysCaps::LocalFileRead) <<
		"&WD=" << encodeInvCap(SysCaps::Windowless) <<
		"&TLS=" << encodeCap(SysCaps::TLS)
	).str();
}

AVM1_GETTER_BODY(AVM1SysCaps, Version)
{
	return act.getSys()->getVersionString();
}

CAPABILITIES_FUNC(HasStreamingAudio, StreamingAudio);
CAPABILITIES_FUNC(HasStreamingVideo, StreamingVideo);
CAPABILITIES_FUNC(HasEmbeddedVideo, EmbeddedVideo);
CAPABILITIES_FUNC(HasPrinting, Printing);
CAPABILITIES_FUNC(HasScreenPlayback, ScreenPlayback);
CAPABILITIES_FUNC(HasScreenBroadcast, ScreenBroadcast);
CAPABILITIES_FUNC(IsDebugger, Debugger);

AVM1_GETTER_BODY(AVM1SysCaps, PlayerType)
{
	std::stringstream s;
	auto engineData = act.getSys()->getEngineData();
	return (s << engineData->getPlayerType()).str();
}

INV_CAPABILITIES_FUNC(AvHardwareDisable, AvHardware);
INV_CAPABILITIES_FUNC(LocalFileReadDisable, LocalFileRead);
INV_CAPABILITIES_FUNC(WindowlessDisable, Windowless);

AVM1_GETTER_BODY(AVM1SysCaps, MaxLevelIDC)
{
	return act.getSys()->getMaxIDCLevel();
}

CAPABILITIES_FUNC(IsEmbeddedInAcrobat, AcrobatEmbedded);

AVM1_GETTER_BODY(AVM1SysCaps, Manufacturer)
{
	return act.getSys()->getEngineData()->getManufacture();
}

AVM1_GETTER_BODY(AVM1SysCaps, OS)
{
	return act.getSys()->getEngineData()->platformOS;
}

AVM1_GETTER_BODY(AVM1SysCaps, CpuArchitecture)
{
	return act.getSys()->getEngineData()->getCPUArch();
}

AVM1_GETTER_BODY(AVM1SysCaps, Language)
{
	auto engineData = act.getSys()->getEngineData();
	return engineData->getLangCode(act.getSys());
}

CAPABILITIES_FUNC(HasIME, IME);
CAPABILITIES_FUNC(Supports32BitProcesses, 32Bit);
CAPABILITIES_FUNC(Supports64BitProcesses, 64Bit);
