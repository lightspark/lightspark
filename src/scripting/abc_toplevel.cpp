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

#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/ASQName.h"
#include "scripting/toplevel/ASString.h"
#include "scripting/toplevel/Date.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/Namespace.h"
#include "scripting/toplevel/Number.h"
#include "scripting/toplevel/Boolean.h"
#include "scripting/toplevel/Integer.h"
#include "scripting/toplevel/UInteger.h"
#include "scripting/toplevel/JSON.h"
#include "scripting/toplevel/Math.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/XML.h"
#include "scripting/toplevel/XMLList.h"
#include "scripting/toplevel/Undefined.h"

#include "scripting/abc.h"
using namespace lightspark;

void ABCVm::registerClassesToplevel(Global* builtin)
{
	builtin->registerBuiltin("Object","",Class<ASObject>::getRef(m_sys));
	builtin->registerBuiltin("Class","",Class_object::getRef(m_sys));
	builtin->registerBuiltin("Number","",Class<Number>::getRef(m_sys));
	builtin->registerBuiltin("Boolean","",Class<Boolean>::getRef(m_sys));
	builtin->setVariableAtomByQName("NaN",nsNameAndKind(),asAtomHandler::fromNumber(m_sys->worker,numeric_limits<double>::quiet_NaN(),true),CONSTANT_TRAIT);
	builtin->setVariableAtomByQName("Infinity",nsNameAndKind(),asAtomHandler::fromNumber(m_sys->worker,numeric_limits<double>::infinity(),true),CONSTANT_TRAIT);
	builtin->registerBuiltin("String","",Class<ASString>::getRef(m_sys));
	builtin->registerBuiltin("Array","",Class<Array>::getRef(m_sys));
	builtin->registerBuiltin("Function","",Class<IFunction>::getRef(m_sys));
	builtin->registerBuiltin("undefined","",_MR(m_sys->getUndefinedRef()));
	builtin->registerBuiltin("Math","",Class<Math>::getRef(m_sys));
	builtin->registerBuiltin("Namespace","",Class<Namespace>::getRef(m_sys));
	builtin->registerBuiltin("AS3","",_MR(Class<Namespace>::getInstanceS(m_sys->worker,BUILTIN_STRINGS::STRING_AS3NS)));
	builtin->registerBuiltin("Date","",Class<Date>::getRef(m_sys));
	builtin->registerBuiltin("JSON","",Class<JSON>::getRef(m_sys));
	builtin->registerBuiltin("RegExp","",Class<RegExp>::getRef(m_sys));
	builtin->registerBuiltin("QName","",Class<ASQName>::getRef(m_sys));
	builtin->registerBuiltin("uint","",Class<UInteger>::getRef(m_sys));
	builtin->registerBuiltin("Error","",Class<ASError>::getRef(m_sys));
	builtin->registerBuiltin("SecurityError","",Class<SecurityError>::getRef(m_sys));
	builtin->registerBuiltin("ArgumentError","",Class<ArgumentError>::getRef(m_sys));
	builtin->registerBuiltin("DefinitionError","",Class<DefinitionError>::getRef(m_sys));
	builtin->registerBuiltin("EvalError","",Class<EvalError>::getRef(m_sys));
	builtin->registerBuiltin("RangeError","",Class<RangeError>::getRef(m_sys));
	builtin->registerBuiltin("ReferenceError","",Class<ReferenceError>::getRef(m_sys));
	builtin->registerBuiltin("SyntaxError","",Class<SyntaxError>::getRef(m_sys));
	builtin->registerBuiltin("TypeError","",Class<TypeError>::getRef(m_sys));
	builtin->registerBuiltin("URIError","",Class<URIError>::getRef(m_sys));
	builtin->registerBuiltin("UninitializedError","",Class<UninitializedError>::getRef(m_sys));
	builtin->registerBuiltin("VerifyError","",Class<VerifyError>::getRef(m_sys));
	if (m_sys->mainClip->usesActionScript3)
	{
		builtin->registerBuiltin("Vector","__AS3__.vec",_MR(Template<Vector>::getTemplate(m_sys->mainClip)));
		builtin->registerBuiltin("XML","",Class<XML>::getRef(m_sys));
		builtin->registerBuiltin("XMLList","",Class<XMLList>::getRef(m_sys));
	}
	builtin->registerBuiltin("int","",Class<Integer>::getRef(m_sys));

	builtin->registerBuiltin("eval","",_MR(m_sys->getBuiltinFunction(eval)));
	builtin->registerBuiltin("print","",_MR(m_sys->getBuiltinFunction(print)));
	builtin->registerBuiltin("trace","",_MR(m_sys->getBuiltinFunction(trace)));
	builtin->registerBuiltin("parseInt","",_MR(m_sys->getBuiltinFunction(parseInt,2,Class<Integer>::getRef(m_sys).getPtr())));
	builtin->registerBuiltin("parseFloat","",_MR(m_sys->getBuiltinFunction(parseFloat,1,Class<Number>::getRef(m_sys).getPtr())));
	builtin->registerBuiltin("encodeURI","",_MR(m_sys->getBuiltinFunction(encodeURI,1,Class<ASString>::getRef(m_sys).getPtr())));
	builtin->registerBuiltin("decodeURI","",_MR(m_sys->getBuiltinFunction(decodeURI,1,Class<ASString>::getRef(m_sys).getPtr())));
	builtin->registerBuiltin("encodeURIComponent","",_MR(m_sys->getBuiltinFunction(encodeURIComponent,1,Class<ASString>::getRef(m_sys).getPtr())));
	builtin->registerBuiltin("decodeURIComponent","",_MR(m_sys->getBuiltinFunction(decodeURIComponent,1,Class<ASString>::getRef(m_sys).getPtr())));
	builtin->registerBuiltin("escape","",_MR(m_sys->getBuiltinFunction(escape,1,Class<ASString>::getRef(m_sys).getPtr())));
	builtin->registerBuiltin("unescape","",_MR(m_sys->getBuiltinFunction(unescape,1,Class<ASString>::getRef(m_sys).getPtr())));
	builtin->registerBuiltin("toString","",_MR(m_sys->getBuiltinFunction(ASObject::_toString,0,Class<ASString>::getRef(m_sys).getPtr())));

	builtin->registerBuiltin("isNaN","",_MR(m_sys->getBuiltinFunction(isNaN,1,Class<Boolean>::getRef(m_sys).getPtr())));
	builtin->registerBuiltin("isFinite","",_MR(m_sys->getBuiltinFunction(isFinite,1,Class<Boolean>::getRef(m_sys).getPtr())));
	builtin->registerBuiltin("isXMLName","",_MR(m_sys->getBuiltinFunction(_isXMLName,1,Class<Boolean>::getRef(m_sys).getPtr())));
}
