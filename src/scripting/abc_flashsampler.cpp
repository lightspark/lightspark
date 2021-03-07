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

#include "scripting/class.h"
#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesFlashSampler(Global* builtin)
{
	builtin->registerBuiltin("clearSamples","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,clearSamples)));
	builtin->registerBuiltin("getGetterInvocationCount","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getGetterInvocationCount)));
	builtin->registerBuiltin("getInvocationCount","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getInvocationCount)));
	builtin->registerBuiltin("getLexicalScopes","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getLexicalScopes)));
	builtin->registerBuiltin("getMasterString","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getMasterString)));
	builtin->registerBuiltin("getMemberNames","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getMemberNames)));
	builtin->registerBuiltin("getSampleCount","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getSampleCount)));
	builtin->registerBuiltin("getSamples","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getSamples)));
	builtin->registerBuiltin("getSavedThis","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getSavedThis)));
	builtin->registerBuiltin("getSetterInvocationCount","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getSetterInvocationCount)));
	builtin->registerBuiltin("getSize","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,getSize)));
	builtin->registerBuiltin("isGetterSetter","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,isGetterSetter)));
	builtin->registerBuiltin("pauseSampling","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,pauseSampling)));
	builtin->registerBuiltin("sampleInternalAllocs","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,sampleInternalAllocs)));
	builtin->registerBuiltin("setSamplerCallback","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,setSamplerCallback)));
	builtin->registerBuiltin("startSampling","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,startSampling)));
	builtin->registerBuiltin("stopSampling","flash.sampler",_MR(Class<IFunction>::getFunction(m_sys,stopSampling)));
	builtin->registerBuiltin("DeleteObjectSample","flash.sampler",Class<DeleteObjectSample>::getRef(m_sys));
	builtin->registerBuiltin("NewObjectSample","flash.sampler",Class<NewObjectSample>::getRef(m_sys));
	builtin->registerBuiltin("Sample","flash.sampler",Class<Sample>::getRef(m_sys));
	builtin->registerBuiltin("StackFrame","flash.sampler",Class<StackFrame>::getRef(m_sys));
}
