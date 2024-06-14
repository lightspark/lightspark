/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/toplevel/ASQName.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Date.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/JSON.h"
#include "scripting/toplevel/Math.h"
#include "scripting/toplevel/Namespace.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/concurrent/Mutex.h"
#include "scripting/flash/concurrent/Condition.h"
#include "scripting/flash/desktop/flashdesktop.h"
#include "scripting/flash/desktop/clipboardformats.h"
#include "scripting/flash/desktop/clipboardtransfermode.h"
#include "scripting/flash/desktop/flashdesktop.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/Bitmap.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/display/bitmapencodingcolorspace.h"
#include "scripting/flash/display/colorcorrection.h"
#include "scripting/flash/display/ColorCorrectionSupport.h"
#include "scripting/flash/display/Graphics.h"
#include "scripting/flash/display/GraphicsBitmapFill.h"
#include "scripting/flash/display/GraphicsEndFill.h"
#include "scripting/flash/display/GraphicsGradientFill.h"
#include "scripting/flash/display/GraphicsPath.h"
#include "scripting/flash/display/GraphicsShaderFill.h"
#include "scripting/flash/display/GraphicsSolidFill.h"
#include "scripting/flash/display/GraphicsStroke.h"
#include "scripting/flash/display/GraphicsTrianglePath.h"
#include "scripting/flash/display/IGraphicsData.h"
#include "scripting/flash/display/IGraphicsFill.h"
#include "scripting/flash/display/IGraphicsPath.h"
#include "scripting/flash/display/IGraphicsStroke.h"
#include "scripting/flash/display/IGraphicsStroke.h"
#include "scripting/flash/display/jpegencoderoptions.h"
#include "scripting/flash/display/jpegxrencoderoptions.h"
#include "scripting/flash/display/pngencoderoptions.h"
#include "scripting/flash/display/shaderparametertype.h"
#include "scripting/flash/display/shaderprecision.h"
#include "scripting/flash/display/swfversion.h"
#include "scripting/flash/display/triangleculling.h"
#include "scripting/flash/display/Loader.h"
#include "scripting/flash/display/LoaderInfo.h"
#include "scripting/flash/display/MorphShape.h"
#include "scripting/flash/display/MovieClip.h"
#include "scripting/flash/display/NativeWindow.h"
#include "scripting/flash/display/Shape.h"
#include "scripting/flash/display/SimpleButton.h"
#include "scripting/flash/display3d/flashdisplay3d.h"
#include "scripting/flash/display3d/flashdisplay3dtextures.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/filesystem/flashfilesystem.h"
#include "scripting/flash/filters/flashfilters.h"
#include "scripting/flash/filters/BevelFilter.h"
#include "scripting/flash/filters/BlurFilter.h"
#include "scripting/flash/filters/ColorMatrixFilter.h"
#include "scripting/flash/filters/ConvolutionFilter.h"
#include "scripting/flash/filters/DisplacementMapFilter.h"
#include "scripting/flash/filters/DropShadowFilter.h"
#include "scripting/flash/filters/GlowFilter.h"
#include "scripting/flash/filters/GradientBevelFilter.h"
#include "scripting/flash/filters/GradientGlowFilter.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/net/URLRequestHeader.h"
#include "scripting/flash/net/URLStream.h" 
#include "scripting/flash/net/Socket.h"
#include "scripting/flash/net/XMLSocket.h"
#include "scripting/flash/net/netgroupreceivemode.h"
#include "scripting/flash/net/netgroupreplicationstrategy.h"
#include "scripting/flash/net/netgroupsendmode.h"
#include "scripting/flash/net/netgroupsendresult.h"
#include "scripting/flash/net/LocalConnection.h"
#include "scripting/flash/net/NetStreamInfo.h"
#include "scripting/flash/net/NetStreamPlayOptions.h"
#include "scripting/flash/net/NetStreamPlayTransitions.h"
#include "scripting/flash/net/DatagramSocket.h"
#include "scripting/flash/printing/flashprinting.h"
#include "scripting/flash/sampler/flashsampler.h"
#include "scripting/flash/security/certificatestatus.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/system/messagechannel.h"
#include "scripting/flash/system/messagechannelstate.h"
#include "scripting/flash/system/securitypanel.h"
#include "scripting/flash/system/systemupdater.h"
#include "scripting/flash/system/systemupdatertype.h"
#include "scripting/flash/system/touchscreentype.h"
#include "scripting/flash/system/ime.h"
#include "scripting/flash/system/jpegloadercontext.h"
#include "scripting/flash/sensors/flashsensors.h"
#include "scripting/flash/utils/flashutils.h"
#include "scripting/flash/utils/ByteArray.h"
#include "scripting/flash/utils/CompressionAlgorithm.h"
#include "scripting/flash/utils/Dictionary.h"
#include "scripting/flash/utils/Proxy.h"
#include "scripting/flash/utils/Timer.h"
#include "scripting/flash/utils/IntervalManager.h"
#include "scripting/flash/utils/IntervalRunner.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/geom/Matrix3D.h"
#include "scripting/flash/geom/orientation3d.h"
#include "scripting/flash/geom/Point.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/geom/Vector3D.h"
#include "scripting/flash/globalization/datetimeformatter.h"
#include "scripting/flash/globalization/datetimestyle.h"
#include "scripting/flash/globalization/collator.h"
#include "scripting/flash/globalization/collatormode.h"
#include "scripting/flash/globalization/datetimenamecontext.h"
#include "scripting/flash/globalization/datetimenamestyle.h"
#include "scripting/flash/globalization/nationaldigitstype.h"
#include "scripting/flash/globalization/lastoperationstatus.h"
#include "scripting/flash/globalization/localeid.h"
#include "scripting/flash/globalization/currencyformatter.h"
#include "scripting/flash/globalization/currencyparseresult.h"
#include "scripting/flash/globalization/numberformatter.h"
#include "scripting/flash/globalization/numberparseresult.h"
#include "scripting/flash/globalization/stringtools.h"
#include "scripting/flash/external/ExternalInterface.h"
#include "scripting/flash/external/ExtensionContext.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/media/audiodecoder.h"
#include "scripting/flash/media/audiooutputchangereason.h"
#include "scripting/flash/media/avnetworkingparams.h"
#include "scripting/flash/media/h264level.h"
#include "scripting/flash/media/h264profile.h"
#include "scripting/flash/media/avtagdata.h"
#include "scripting/flash/media/id3info.h"
#include "scripting/flash/media/microphoneenhancedmode.h"
#include "scripting/flash/media/microphoneenhancedoptions.h"
#include "scripting/flash/media/soundcodec.h"
#include "scripting/flash/media/stagevideoavailabilityreason.h"
#include "scripting/flash/media/videodecoder.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/text/csmsettings.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/text/flashtextengine.h"
#include "scripting/flash/text/textrenderer.h"
#include "scripting/flash/ui/Keyboard.h"
#include "scripting/flash/ui/Mouse.h"
#include "scripting/flash/ui/Multitouch.h"
#include "scripting/flash/ui/ContextMenu.h"
#include "scripting/flash/ui/ContextMenuItem.h"
#include "scripting/flash/ui/ContextMenuBuiltInItems.h"
#include "scripting/flash/ui/gameinput.h"
#include "scripting/avmplus/avmplus.h"
#include "scripting/avm1/avm1key.h"
#include "scripting/avm1/avm1sound.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/avm1media.h"
#include "scripting/avm1/avm1net.h"
#include "scripting/avm1/avm1text.h"
#include "scripting/avm1/avm1ui.h"
#include "scripting/avm1/avm1xml.h"
#include "scripting/avm1/avm1array.h"

using namespace lightspark;

//Orrible preprocessor hack:
//We need to assign a unique, progressive ID to each actionscript class
//The idea is to write the info about all classes in allclasses.h
//The file will be included twice with different definitions of REGISTER_CLASS_NAME and REGISTER_CLASS_NAME2
//The first time an enumeration will be build, the second time it will be used to assign the values

//Phase 1: build an enumeration
#define REGISTER_CLASS_NAME(TYPE, NS) \
	CLASS_##TYPE,

#define REGISTER_CLASS_NAME2(TYPE,NAME,NS) \
	CLASS_##TYPE,

enum ASClassIds
{
//Leave a space for the special Class class
CLASS_CLASS=0,
#include "allclasses.h"
CLASS_LAST
};

#undef REGISTER_CLASS_NAME
#undef REGISTER_CLASS_NAME2

//Phase 2: use the enumeratio to assign unique ids
#define REGISTER_CLASS_NAME(TYPE, NS) \
	template<> const char* ClassName<TYPE>::name = #TYPE; \
	template<> const char* ClassName<TYPE>::ns = NS; \
	template<> unsigned ClassName<TYPE>::id = CLASS_##TYPE;

#define REGISTER_CLASS_NAME2(TYPE,NAME,NS) \
	template<> const char* ClassName<TYPE>::name = NAME; \
	template<> const char* ClassName<TYPE>::ns = NS; \
	template<> unsigned int ClassName<TYPE>::id = CLASS_##TYPE;

#include "allclasses.h"

//Define a variable to let outside code know the number of defined classes
uint32_t asClassCount = CLASS_LAST;
