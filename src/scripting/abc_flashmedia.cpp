/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/media/audiodecoder.h"
#include "scripting/flash/media/audiooutputchangereason.h"
#include "scripting/flash/media/avnetworkingparams.h"
#include "scripting/flash/media/h264level.h"
#include "scripting/flash/media/h264profile.h"
#include "scripting/flash/media/microphoneenhancedmode.h"
#include "scripting/flash/media/microphoneenhancedoptions.h"
#include "scripting/flash/media/soundcodec.h"
#include "scripting/flash/media/stagevideoavailabilityreason.h"
#include "scripting/flash/media/videodecoder.h"
#include "scripting/flash/media/avtagdata.h"
#include "scripting/flash/media/id3info.h"

#include "scripting/toplevel/Global.h"
#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;


void ABCVm::registerClassesFlashMedia(Global* builtin)
{
	builtin->registerBuiltin("AudioDecoder","flash.media",Class<MediaAudioDecoder>::getRef(m_sys));
	builtin->registerBuiltin("AudioOutputChangeReason","flash.media",Class<AudioOutputChangeReason>::getRef(m_sys));
	builtin->registerBuiltin("AVNetworkingParams","flash.media",Class<AVNetworkingParams>::getRef(m_sys));
	builtin->registerBuiltin("H264Level","flash.media",Class<H264Level>::getRef(m_sys));
	builtin->registerBuiltin("H264Profile","flash.media",Class<H264Profile>::getRef(m_sys));
	builtin->registerBuiltin("MicrophoneEnhancedMode","flash.media",Class<MicrophoneEnhancedMode>::getRef(m_sys));
	builtin->registerBuiltin("MicrophoneEnhancedOptions","flash.media",Class<MicrophoneEnhancedOptions>::getRef(m_sys));
	builtin->registerBuiltin("SoundCodec","flash.media",Class<SoundCodec>::getRef(m_sys));
	builtin->registerBuiltin("StageVideoAvailabilityReason","flash.media",Class<StageVideoAvailabilityReason>::getRef(m_sys));
	builtin->registerBuiltin("VideoCodec","flash.media",Class<MediaVideoDecoder>::getRef(m_sys));
	builtin->registerBuiltin("SoundTransform","flash.media",Class<SoundTransform>::getRef(m_sys));
	builtin->registerBuiltin("Video","flash.media",Class<Video>::getRef(m_sys));
	builtin->registerBuiltin("Sound","flash.media",Class<Sound>::getRef(m_sys));
	builtin->registerBuiltin("SoundLoaderContext","flash.media",Class<SoundLoaderContext>::getRef(m_sys));
	builtin->registerBuiltin("SoundChannel","flash.media",Class<SoundChannel>::getRef(m_sys));
	builtin->registerBuiltin("SoundMixer","flash.media",Class<SoundMixer>::getRef(m_sys));
	builtin->registerBuiltin("StageVideo","flash.media",Class<StageVideo>::getRef(m_sys));
	builtin->registerBuiltin("StageVideoAvailability","flash.media",Class<StageVideoAvailability>::getRef(m_sys));
	builtin->registerBuiltin("VideoStatus","flash.media",Class<VideoStatus>::getRef(m_sys));
	builtin->registerBuiltin("Microphone","flash.media",Class<Microphone>::getRef(m_sys));
	builtin->registerBuiltin("Camera","flash.media",Class<Camera>::getRef(m_sys));
	builtin->registerBuiltin("VideoStreamSettings","flash.media",Class<VideoStreamSettings>::getRef(m_sys));
	builtin->registerBuiltin("H264VideoStreamSettings","flash.media",Class<H264VideoStreamSettings>::getRef(m_sys));
	builtin->registerBuiltin("AVTagData","flash.media",Class<AVTagData>::getRef(m_sys));
	builtin->registerBuiltin("ID3Info","flash.media",Class<ID3Info>::getRef(m_sys));
}
