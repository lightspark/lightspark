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

#include "scripting/flash/net/flashnet.h"
#include "scripting/flash/net/URLRequestHeader.h"
#include "scripting/flash/net/URLStream.h"
#include "scripting/flash/net/XMLSocket.h"
#include "scripting/flash/net/NetStreamInfo.h"
#include "scripting/flash/net/NetStreamPlayOptions.h"
#include "scripting/flash/net/NetStreamPlayTransitions.h"
#include "scripting/flash/net/netgroupreceivemode.h"
#include "scripting/flash/net/netgroupreplicationstrategy.h"
#include "scripting/flash/net/netgroupsendmode.h"
#include "scripting/flash/net/netgroupsendresult.h"
#include "scripting/flash/net/DatagramSocket.h"
#include "scripting/flash/net/LocalConnection.h"

#include "scripting/toplevel/Global.h"
#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesFlashNet(Global* builtin)
{
	builtin->registerBuiltin("navigateToURL","flash.net",_MR(m_sys->getBuiltinFunction(navigateToURL)));
	builtin->registerBuiltin("sendToURL","flash.net",_MR(m_sys->getBuiltinFunction(sendToURL)));
	builtin->registerBuiltin("DynamicPropertyOutput","flash.net",Class<DynamicPropertyOutput>::getRef(m_sys));
	builtin->registerBuiltin("FileFilter","flash.net",Class<FileFilter>::getRef(m_sys));
	builtin->registerBuiltin("FileReference","flash.net",Class<FileReference>::getRef(m_sys));
	builtin->registerBuiltin("IDynamicPropertyWriter","flash.net",InterfaceClass<IDynamicPropertyWriter>::getRef(m_sys));
	builtin->registerBuiltin("IDynamicPropertyOutput","flash.net",InterfaceClass<IDynamicPropertyOutput>::getRef(m_sys));
	builtin->registerBuiltin("LocalConnection","flash.net",Class<LocalConnection>::getRef(m_sys));
	builtin->registerBuiltin("NetConnection","flash.net",Class<NetConnection>::getRef(m_sys));
	builtin->registerBuiltin("NetGroup","flash.net",Class<NetGroup>::getRef(m_sys));
	builtin->registerBuiltin("NetStream","flash.net",Class<NetStream>::getRef(m_sys));
	builtin->registerBuiltin("NetStreamAppendBytesAction","flash.net",Class<NetStreamAppendBytesAction>::getRef(m_sys));
	builtin->registerBuiltin("NetStreamInfo","flash.net",Class<NetStreamInfo>::getRef(m_sys));
	builtin->registerBuiltin("NetStreamPlayOptions","flash.net",Class<NetStreamPlayOptions>::getRef(m_sys));
	builtin->registerBuiltin("NetStreamPlayTransitions","flash.net",Class<NetStreamPlayTransitions>::getRef(m_sys));
	builtin->registerBuiltin("URLLoader","flash.net",Class<URLLoader>::getRef(m_sys));
	builtin->registerBuiltin("URLStream","flash.net",Class<URLStream>::getRef(m_sys));
	builtin->registerBuiltin("URLLoaderDataFormat","flash.net",Class<URLLoaderDataFormat>::getRef(m_sys));
	builtin->registerBuiltin("URLRequest","flash.net",Class<URLRequest>::getRef(m_sys));
	builtin->registerBuiltin("URLRequestHeader","flash.net",Class<URLRequestHeader>::getRef(m_sys));
	builtin->registerBuiltin("URLRequestMethod","flash.net",Class<URLRequestMethod>::getRef(m_sys));
	builtin->registerBuiltin("URLVariables","flash.net",Class<URLVariables>::getRef(m_sys));
	builtin->registerBuiltin("SharedObject","flash.net",Class<SharedObject>::getRef(m_sys));
	builtin->registerBuiltin("SharedObjectFlushStatus","flash.net",Class<SharedObjectFlushStatus>::getRef(m_sys));
	builtin->registerBuiltin("ObjectEncoding","flash.net",Class<ObjectEncoding>::getRef(m_sys));
	builtin->registerBuiltin("Socket","flash.net",Class<ASSocket>::getRef(m_sys));
	builtin->registerBuiltin("Responder","flash.net",Class<Responder>::getRef(m_sys));
	builtin->registerBuiltin("XMLSocket","flash.net",Class<XMLSocket>::getRef(m_sys));
	builtin->registerBuiltin("registerClassAlias","flash.net",_MR(m_sys->getBuiltinFunction(registerClassAlias)));
	builtin->registerBuiltin("getClassByAlias","flash.net",_MR(m_sys->getBuiltinFunction(getClassByAlias)));
	builtin->registerBuiltin("NetGroupReceiveMode","flash.net",Class<NetGroupReceiveMode>::getRef(m_sys));
	builtin->registerBuiltin("NetGroupReplicationStrategy","flash.net",Class<NetGroupReplicationStrategy>::getRef(m_sys));
	builtin->registerBuiltin("NetGroupSendMode","flash.net",Class<NetGroupSendMode>::getRef(m_sys));
	builtin->registerBuiltin("NetGroupSendResult","flash.net",Class<NetGroupSendResult>::getRef(m_sys));
	if(m_sys->flashMode==SystemState::AIR)
		builtin->registerBuiltin("DatagramSocket","flash.net",Class<DatagramSocket>::getRef(m_sys));

	builtin->registerBuiltin("DRMManager","flash.net.drm",Class<DRMManager>::getRef(m_sys));
}
