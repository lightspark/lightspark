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

#include "scripting/flash/sampler/flashsampler.h"

#include "scripting/toplevel/Global.h"
#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesFlashSampler(Global* builtin)
{
	builtin->registerBuiltin("clearSamples","flash.sampler",_MR(m_sys->getBuiltinFunction(clearSamples)));
	builtin->registerBuiltin("getGetterInvocationCount","flash.sampler",_MR(m_sys->getBuiltinFunction(getGetterInvocationCount)));
	builtin->registerBuiltin("getInvocationCount","flash.sampler",_MR(m_sys->getBuiltinFunction(getInvocationCount)));
	builtin->registerBuiltin("getLexicalScopes","flash.sampler",_MR(m_sys->getBuiltinFunction(getLexicalScopes)));
	builtin->registerBuiltin("getMasterString","flash.sampler",_MR(m_sys->getBuiltinFunction(getMasterString)));
	builtin->registerBuiltin("getMemberNames","flash.sampler",_MR(m_sys->getBuiltinFunction(getMemberNames)));
	builtin->registerBuiltin("getSampleCount","flash.sampler",_MR(m_sys->getBuiltinFunction(getSampleCount)));
	builtin->registerBuiltin("getSamples","flash.sampler",_MR(m_sys->getBuiltinFunction(getSamples)));
	builtin->registerBuiltin("getSavedThis","flash.sampler",_MR(m_sys->getBuiltinFunction(getSavedThis)));
	builtin->registerBuiltin("getSetterInvocationCount","flash.sampler",_MR(m_sys->getBuiltinFunction(getSetterInvocationCount)));
	builtin->registerBuiltin("getSize","flash.sampler",_MR(m_sys->getBuiltinFunction(getSize)));
	builtin->registerBuiltin("isGetterSetter","flash.sampler",_MR(m_sys->getBuiltinFunction(isGetterSetter)));
	builtin->registerBuiltin("pauseSampling","flash.sampler",_MR(m_sys->getBuiltinFunction(pauseSampling)));
	builtin->registerBuiltin("sampleInternalAllocs","flash.sampler",_MR(m_sys->getBuiltinFunction(sampleInternalAllocs)));
	builtin->registerBuiltin("setSamplerCallback","flash.sampler",_MR(m_sys->getBuiltinFunction(setSamplerCallback)));
	builtin->registerBuiltin("startSampling","flash.sampler",_MR(m_sys->getBuiltinFunction(startSampling)));
	builtin->registerBuiltin("stopSampling","flash.sampler",_MR(m_sys->getBuiltinFunction(stopSampling)));
	builtin->registerBuiltin("DeleteObjectSample","flash.sampler",Class<DeleteObjectSample>::getRef(m_sys));
	builtin->registerBuiltin("NewObjectSample","flash.sampler",Class<NewObjectSample>::getRef(m_sys));
	builtin->registerBuiltin("Sample","flash.sampler",Class<Sample>::getRef(m_sys));
	builtin->registerBuiltin("StackFrame","flash.sampler",Class<StackFrame>::getRef(m_sys));
}
