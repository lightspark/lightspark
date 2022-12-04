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

#include "scripting/flash/media/avnetworkingparams.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void AVNetworkingParams::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_SEALED|CLASS_FINAL);

	REGISTER_GETTER_SETTER(c,appendRandomQueryParameter);
	REGISTER_GETTER_SETTER(c,forceNativeNetworking);
	REGISTER_GETTER_SETTER(c,networkDownVerificationUrl);
	REGISTER_GETTER_SETTER(c,readSetCookieHeader);
	REGISTER_GETTER_SETTER(c,useCookieHeaderForAllRequests);
}

ASFUNCTIONBODY_ATOM(AVNetworkingParams,_constructor)
{
    AVNetworkingParams* th =asAtomHandler::as<AVNetworkingParams>(obj);
    bool init_forceNativeNetworking = false;
    bool init_readSetCookieHeader = true;
    bool init_useCookieHeaderForAllRequests = false;
    tiny_string init_networkDownVerificationUrl;
	ARG_CHECK(ARG_UNPACK(init_forceNativeNetworking,false)(init_readSetCookieHeader,true)(init_useCookieHeaderForAllRequests,false)(init_networkDownVerificationUrl,""));
    th->forceNativeNetworking = init_forceNativeNetworking;
    th->readSetCookieHeader = init_readSetCookieHeader;
    th->useCookieHeaderForAllRequests = init_useCookieHeaderForAllRequests;
    th->networkDownVerificationUrl = init_networkDownVerificationUrl;
}

ASFUNCTIONBODY_GETTER_SETTER(AVNetworkingParams,appendRandomQueryParameter)
ASFUNCTIONBODY_GETTER_SETTER(AVNetworkingParams,forceNativeNetworking)
ASFUNCTIONBODY_GETTER_SETTER(AVNetworkingParams,networkDownVerificationUrl)
ASFUNCTIONBODY_GETTER_SETTER(AVNetworkingParams,readSetCookieHeader)
ASFUNCTIONBODY_GETTER_SETTER(AVNetworkingParams,useCookieHeaderForAllRequests)
