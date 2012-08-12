/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2012  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Date.h"
#include "scripting/toplevel/Math.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/flash/accessibility/flashaccessibility.h"
#include "scripting/flash/desktop/flashdesktop.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/BitmapData.h"
#include "scripting/flash/events/flashevents.h"
#include "scripting/flash/filters/flashfilters.h"
#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/net/URLRequestHeader.h"
#include "scripting/flash/net/URLStream.h"
#include "scripting/flash/net/XMLSocket.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/sensors/flashsensors.h"
#include "scripting/flash/utils/flashutils.h"
#include "scripting/flash/geom/flashgeom.h"
#include "scripting/flash/external/ExternalInterface.h"
#include "scripting/flash/media/flashmedia.h"
#include "scripting/flash/xml/flashxml.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/flash/text/flashtext.h"
#include "scripting/flash/text/flashtextengine.h"

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
