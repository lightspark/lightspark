/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2008-2013  Alessandro Pignotti (a.pignotti@sssup.it)

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
#include "swftypes.h"
#include "scripting/abc.h"
#include "scripting/class.h"
#include "scripting/flash/geom/Rectangle.h"
#include "scripting/flash/display/flashdisplay.h"
#include "scripting/flash/display/FrameContainer.h"
#include "scripting/flash/display/RootMovieClip.h"
#include "scripting/flash/system/flashsystem.h"
#include "scripting/flash/errors/flasherrors.h"
#include "scripting/toplevel/toplevel.h"
#include "scripting/toplevel/AVM1Function.h"
#include "scripting/toplevel/Global.h"
#include "scripting/toplevel/Number.h"
#include "scripting/avm1/avm1display.h"
#include "scripting/avm1/avm1array.h"
#include "scripting/avm1/scope.h"
#include "parsing/tags.h"
#include "backends/audio.h"

using namespace std;
using namespace lightspark;

void ACTIONRECORD::PushStack(std::stack<asAtom> &stack, const asAtom &a)
{
	stack.push(a);
}

asAtom ACTIONRECORD::PopStack(std::stack<asAtom>& stack)
{
	if (stack.empty())
		return asAtomHandler::undefinedAtom;
	asAtom ret = stack.top();
	stack.pop();
	return ret;
}
asAtom ACTIONRECORD::PeekStack(std::stack<asAtom>& stack)
{
	if (stack.empty())
		throw RunTimeException("AVM1: empty stack");
	return stack.top();
}
AVM1context::AVM1context():
	scope(nullptr),
	globalScope(nullptr),
	keepLocals(true),
	callDepth(0),
	actionsExecuted(0),
	swfversion(0),
	exceptionthrown(nullptr),
	callee(nullptr)
{
}

AVM1context::AVM1context(DisplayObject* target, SystemState* sys) :
	globalScope(new AVM1Scope(sys->avm1global)),
	keepLocals(true),
	callDepth(0),
	actionsExecuted(0),
	swfversion(target->loadedFrom->version),
	exceptionthrown(nullptr),
	callee(nullptr)
{
	globalScope->addStoredMember();
	scope = new AVM1Scope(globalScope, target);
	scope->addStoredMember();
}

AVM1context::~AVM1context()
{
	if (scope)
	{
		scope->removeStoredMember();
		scope->decRef();
	}
	if (globalScope)
	{
		globalScope->removeStoredMember();
		globalScope->decRef();
	}
}

void AVM1context::setScope(AVM1Scope* sc)
{
	if (scope)
	{
		scope->removeStoredMember();
		scope->decRef();
	}
	scope = sc;
	if (scope)
	{
		scope->addStoredMember();
	}
}
void AVM1context::setGlobalScope(AVM1Scope* sc)
{
	if (globalScope)
	{
		globalScope->removeStoredMember();
		globalScope->decRef();
	}
	globalScope = sc;
	if (globalScope)
	{
		globalScope->addStoredMember();
	}
}

// Based on Ruffle's `avm1::activation::Activation::resolve_target_path()`.
ASObject* AVM1context::resolveTargetPath
(
	const asAtom& thisObj,
	DisplayObject* baseClip,
	DisplayObject* root,
	ASObject* start,
	const tiny_string& _path,
	bool hasSlash,
	bool first
) const
{
	auto path = _path;
	// An empty path resolves to the starting clip.
	if (path.empty())
	{
		if (start)
			start->incRef();
		return start;
	}

	ASObject* obj = start;
	bool isSlashPath;
	// A starting `/` means we have an absolute path, starting from the
	// root. e.g. `/foo` == `_root.foo`.
	if ((isSlashPath = path.startsWith("/")))
	{
		path = path.substr(1, UINT32_MAX);
		obj = root;
	}
	bool destroyprev=false;
	GET_VARIABLE_RESULT varres = GET_VARIABLE_RESULT::GETVAR_NORMAL;
	while (!path.empty())
	{
		destroyprev = varres & GET_VARIABLE_RESULT::GETVAR_ISINCREFFED;
		varres = GET_VARIABLE_RESULT::GETVAR_NORMAL;
		// Skip over any leading `:`s.
		// e.g. `:foo`, and `::foo` are the same as `foo`.
		path = path.trimStartMatches(':');

		asAtom val = asAtomHandler::invalidAtom;
		auto clip = obj && obj->is<DisplayObject>() ? obj->as<DisplayObject>() : nullptr;

		uint32_t ch = path.numChars() >= 3 ? path[2] : '\0';
		// Check for an SWF 4 `_parent` path (`..[/:]`).
		if (path.startsWith("..") && (ch == '\0' || ch == '/' || ch == ':'))
		{
			// SWF 4 style `_parent` path.
			isSlashPath = ch == '/';
			path = path.stripPrefix("..", ch != '\0');

			if (clip == nullptr || clip->getParent() == nullptr)
			{
				// Tried to get the parent of a root clip, bail early.
				return nullptr;
			}

			bool isStage = clip->getParent()->is<Stage>();
			auto parent = isStage ? clip->AVM1getRoot() : clip->getParent();

			val = asAtomHandler::fromObject(parent);
		}
		else
		{
			// Find the next delimiter.
			// `:`, `.`, and `/` are all valid path delimiters, with the
			// only restriction being that a `.` isn't considered a valid
			// delimiter after a `/` appears.
			size_t i;
			bool done;
			for (i = 0, done = false; i < path.numChars(); ++i)
			{
				switch (path[i])
				{
					case '/': isSlashPath = true;
					// Falls through.
					case ':': done = true; break;
					case '.': done = !isSlashPath; break;
					default: done = false; break;
				}
				if (done)
					break;
			}

			auto name = path.substr(0, i);
			path = path.substr(std::min(uint32_t(i + 1), path.numChars()), UINT32_MAX);

			if (first && name == "this")
			{
				if (asAtomHandler::is<AVM1Super_object>(thisObj))
					val = asAtomHandler::fromObjectNoPrimitive(asAtomHandler::as<AVM1Super_object>(thisObj)->getBaseObject());
				else
					val = thisObj;
			}
			else if (first && name == "_root")
				val = asAtomHandler::fromObject(baseClip->AVM1getRoot());
			else
			{
				// Try to get the value from the object.
				// NOTE: This resolves `DisplayObject`s first, and then
				// locals, which is the opposite of what `ActionGetMember`'s
				// property access does.
				auto child =
				(
					obj->is<DisplayObjectContainer>() ?
					obj->as<DisplayObjectContainer>()->getLegacyChildByName
					(
						name,
						isCaseSensitive()
					) : nullptr
				);

				if (child != nullptr)
				{
					// NOTE: If the object can't be represented as an
					// AVM1 object, such as `Shape`, then any attempt to
					// access it will return the parent instead.
					if (!hasSlash && child->is<Shape>())
						val = asAtomHandler::fromObject(child->getParent());
					else
						val = asAtomHandler::fromObject(child);
				}
				else
				{
					auto sys = baseClip->getSystemState();
					multiname objName(nullptr);
					objName.name_type = multiname::NAME_STRING;
					objName.name_s_id = sys->getUniqueStringId(name, isCaseSensitive());
					varres = obj->AVM1getVariableByMultiname
					(
						val,
						objName,
						GET_VARIABLE_OPTION::NO_INCREF,
						baseClip->getInstanceWorker(),
						hasSlash
					);
				}
			}
			if (destroyprev)
				obj->decRef();
		}

		// NOTE: `this`, and `_root` are only allowed at the start of
		// the path.
		first = false;

		// Convert the atom into an object, while we're traversing the path.
		if (asAtomHandler::isObjectPtr(val))
			obj = asAtomHandler::getObjectNoCheck(val);
		else
			return nullptr;
	}
	if (obj && ((varres & GET_VARIABLE_RESULT::GETVAR_ISINCREFFED)==0))
		obj->incRef();
	return obj;
}

// Based on Ruffle's `avm1::activation::Activation::resolve_variable_path()`.
std::pair<ASObject*, tiny_string> AVM1context::resolveVariablePath
(
	const asAtom& thisObj,
	DisplayObject* baseClip,
	const _R<DisplayObject>& clip,
	const tiny_string& path
) const
{
	auto sys = clip->getSystemState();
	auto wrk = clip->getInstanceWorker();

	bool pathHasSlash = path.contains('/');

	// Find the last `:`, or `.` in the path.
	// If we've got one, then it must be resolved as a target path.
	auto pos = path.findLast(":.");
	if (pos != tiny_string::npos)
	{
		// We've got a `:`, or `.`, meaning it's an object path, and a
		// variable name. Which has to be resolved directly on the target
		// object.
		auto varPath = path.substr(0, pos);
		auto varName = path.substr(pos + 1, UINT32_MAX);
		auto root = clip->AVM1getRoot();

		multiname m(nullptr);
		m.name_type = multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id = sys->getUniqueStringId(varName, isCaseSensitive());

		ASObject* obj = nullptr;
		scope->forEachScope([&](AVM1Scope& scope)
		{
			obj = resolveTargetPath
			(
				thisObj,
				baseClip,
				root,
				scope.getLocalsPtr(),
				varPath,
				pathHasSlash
			);

			if (obj == nullptr || !obj->hasPropertyByMultiname(m, true, true, wrk))
			{
				if (obj != nullptr)
					obj->decRef();
				obj = nullptr;
				return true;
			}

			return false;
		});
		if (obj != nullptr)
			return std::make_pair(obj, varName);
		return std::make_pair(nullptr, "");
	}

	// It's a normal variable name.
	// Return the starting clip, and path.
	clip->incRef();
	return std::make_pair(clip.getPtr(), path);
}

// Based on Ruffle's `avm1::activation::Activation::get_variable()`.
asAtom AVM1context::getVariable
(
	const asAtom& thisObj,
	DisplayObject* baseClip,
	DisplayObject* clip,
	const tiny_string& path
) const
{
	// Try to resolve a variable path for `ActionGetVariable`.
	if (clip == nullptr)
		clip = baseClip->AVM1getRoot();

	auto sys = clip->getSystemState();
	auto wrk = clip->getInstanceWorker();

	bool pathHasSlash = path.contains('/');

	// Find the last `:`, or `.` in the path.
	// If we've got one, then it must be resolved as a target path.
	auto pos = path.findLast(":.");
	if (pos != tiny_string::npos)
	{
		// We've got a `:`, or `.`, meaning it's an object path, and a
		// variable name. Which has to be resolved directly on the target
		// object.
		auto varPath = path.substr(0, pos);
		auto varName = path.substr(pos + 1, UINT32_MAX);
		auto root = clip->AVM1getRoot();

		multiname m(nullptr);
		m.name_type = multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id = sys->getUniqueStringId(varName, isCaseSensitive());

		asAtom ret = asAtomHandler::invalidAtom;
		scope->forEachScope([&](AVM1Scope& scope)
		{
			auto obj = resolveTargetPath
			(
				thisObj,
				baseClip,
				root,
				scope.getLocalsPtr(),
				varPath,
				pathHasSlash
			);

			if (obj == nullptr || !obj->hasPropertyByMultiname(m, true, true, wrk))
			{
				if (obj != nullptr)
					obj->decRef();
				return true;
			}

			obj->AVM1getVariableByMultiname
			(
				ret,
				m,
				GET_VARIABLE_OPTION::NONE,
				wrk
			);
			obj->decRef();
			return false;
		});
		if (asAtomHandler::isValid(ret))
			return ret;
		return asAtomHandler::undefinedAtom;
	}

	// If the path doesn't have a trailing variable, it could still be a
	// slash path.
	if (pathHasSlash)
	{
		auto root = clip->AVM1getRoot();

		asAtom ret = asAtomHandler::invalidAtom;
		scope->forEachScope([&](AVM1Scope& scope)
		{
			auto obj = resolveTargetPath
			(
				thisObj,
				baseClip,
				root,
				scope.getLocalsPtr(),
				path,
				true,
				pathHasSlash
			);

			if (obj == nullptr)
				return true;

			ret = asAtomHandler::fromObject(obj);
			return false;
		});

		if (asAtomHandler::isValid(ret))
			return ret;
	}

	if (path == "this")
	{
		asAtom res = thisObj;
		if (asAtomHandler::is<AVM1Super_object>(thisObj))
			res = asAtomHandler::fromObjectNoPrimitive(asAtomHandler::as<AVM1Super_object>(thisObj)->getBaseObject());
		ASATOM_INCREF(res);
		return res;
	}

	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.isAttribute = false;
	m.name_s_id = sys->getUniqueStringId
	(
		path,
		isCaseSensitive()
	);

	// It's a normal variable name.
	// Resolve it using the scope chain.
	asAtom atom = asAtomHandler::invalidAtom;
	if (scope)
		atom = scope->getVariableByMultiname
			   (
				   baseClip,
				   m,
				   GET_VARIABLE_OPTION::NONE,
				   wrk
			   );

	if (asAtomHandler::isValid(atom))
		return atom;

	// If variable resolution on the scope chain failed, then fallback
	// to looking in either the target, or root clip.
	if (!clip->hasPropertyByMultiname(m, true, true, wrk))
	{
		if (!isCaseSensitive() && clip->getSystemState()->avm1global)
		{
			// variable not found for swf <= 6, try to find builtin class with case sensitive name
			m.name_s_id = sys->getUniqueStringId(path, true);
			asAtom ret = asAtomHandler::invalidAtom;
			clip->getSystemState()->avm1global->getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::NONE,wrk);
			if (asAtomHandler::isValid(ret))
				return ret;
		}
		return asAtomHandler::undefinedAtom;
	}

	clip->AVM1getVariableByMultiname
	(
		atom,
		m,
		GET_VARIABLE_OPTION::NONE,
		wrk
	);

	if (asAtomHandler::isValid(atom))
		return atom;
	return asAtomHandler::undefinedAtom;
}

// Based on Ruffle's `avm1::activation::Activation::set_variable()`.
void AVM1context::setVariable
(
	asAtom& thisObj,
	DisplayObject* baseClip,
	DisplayObject* clip,
	const tiny_string& path,
	const asAtom& value
)
{
	// Try to resolve a variable path for `ActionSetVariable`.
	if (clip == nullptr)
		clip = baseClip->AVM1getRoot();

	auto sys = clip->getSystemState();
	auto wrk = clip->getInstanceWorker();

	bool pathHasSlash = path.contains('/');

	// If the path is empty, default to using the root clip for the
	// variable path.
	if (path.empty())
		return;

	// Special case for mutating `this`.
	if (path == "this")
	{
		ASATOM_DECREF(thisObj);
		asAtom res = value;
		if (asAtomHandler::is<AVM1Super_object>(value))
			res = asAtomHandler::fromObjectNoPrimitive(asAtomHandler::as<AVM1Super_object>(value)->getBaseObject());
		ASATOM_INCREF(res);
		thisObj = res;
		return;
	}

	// Find the last `:`, or `.` in the path.
	// If we've got one, then it must be resolved as a target path.
	auto pos = path.findLast(":.");
	if (pos != tiny_string::npos)
	{
		// We've got a `:`, or `.`, meaning it's an object path, and a
		// variable name. Which has to be resolved directly on the target
		// object.
		auto varPath = path.substr(0, pos);
		auto varName = path.substr(pos + 1, UINT32_MAX);
		auto root = clip->AVM1getRoot();

		multiname m(nullptr);
		m.name_type = multiname::NAME_STRING;
		m.isAttribute = false;
		m.name_s_id = sys->getUniqueStringId(varName, isCaseSensitive());

		scope->forEachScope([&](AVM1Scope& scope)
		{
			auto obj = resolveTargetPath
			(
				thisObj,
				baseClip,
				root,
				scope.getLocalsPtr(),
				varPath,
				true,
				pathHasSlash
			);

			if (obj == nullptr || !obj->hasPropertyByMultiname(m, true, true, wrk))
			{
				if (obj)
					obj->decRef();
				return true;
			}

			bool alreadySet = obj->AVM1setVariableByMultiname
			(
				m,
				(asAtom&)value,
				CONST_ALLOWED,
				wrk
			);
			if (alreadySet)
				ASATOM_DECREF(value)
			obj->decRef();
			return false;
		});

		return;
	}

	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.isAttribute = false;
	m.name_s_id = sys->getUniqueStringId(path, isCaseSensitive());

	// It's a normal variable name. Set it using the scope chain.
	// This will overwrite the value, if the property already exists in
	// the scope chain, otherwise, it's created on the top level object.
	bool alreadySet = scope->setVariableByMultiname
	(
		m,
		(asAtom&)value,
		CONST_ALLOWED,
		wrk
	);

	if (alreadySet)
		ASATOM_DECREF(value)
}

struct tryCatchBlock
{
	uint16_t trysize;
	uint16_t catchsize;
	uint16_t finallysize;
	uint8_t reg=UINT8_MAX;
	tiny_string name;
	uint32_t startpos;
};
bool ACTIONRECORD::implementsInterface(asAtom type, ASObject* value, ASWorker* wrk)
{
	bool implementsinterface=false;
	if (asAtomHandler::is<AVM1Function>(type))
	{
		ASObject* pr = value->getprop_prototype();
		if (pr)
		{
			asAtom o = asAtomHandler::invalidAtom;
			multiname m(nullptr);
			m.name_type=multiname::NAME_STRING;
			m.name_s_id=BUILTIN_STRINGS::STRING_CONSTRUCTOR;
			pr->getVariableByMultiname(o,m,GET_VARIABLE_OPTION(SKIP_IMPL|NO_INCREF),wrk);
			if (asAtomHandler::is<AVM1Function>(o))
				implementsinterface = asAtomHandler::as<AVM1Function>(o)->implementsInterface(type);
		}
	}
	else if (value->is<Array>()
			   && (asAtomHandler::getObject(type) == Class<AVM1Array>::getRef(wrk->getSystemState()).getPtr()
				   || asAtomHandler::getObject(type) == Class<Array>::getRef(wrk->getSystemState()).getPtr()))
		implementsinterface = true; // special case comparing AVM1Array to Array
	else
		implementsinterface = ABCVm::instanceOf(value,asAtomHandler::getObject(type));
	return implementsinterface;
}
Mutex executeactionmutex;
void ACTIONRECORD::executeActions(DisplayObject *clip, AVM1context* context, const std::vector<uint8_t> &actionlist,
								  uint32_t startactionpos, AVM1Scope* scope, bool fromInitAction,
								  asAtom* result, asAtom* obj, asAtom *args, uint32_t num_args,
								  const std::vector<uint32_t>& paramnames,
								  const std::vector<uint8_t>& paramregisternumbers,
								  bool preloadParent, bool preloadRoot, bool suppressSuper,
								  bool preloadSuper, bool suppressArguments, bool preloadArguments,
								  bool suppressThis, bool preloadThis, bool preloadGlobal,
								  AVM1Function *caller, AVM1Function *callee,
								  asAtom *superobj,bool isInternalCall)
{
	Locker l(executeactionmutex);
	bool clip_isTarget=false;
	bool invalidTarget=false;
	assert(!clip->needsActionScript3());
	SystemState* sys = clip->getSystemState();
	ASWorker* wrk = sys->worker;
	wrk->AVM1_cur_recursion_function++;
	if (isInternalCall)
		wrk->AVM1_cur_recursion_internal++;

	if (context->callDepth == 0)
		context->startTime = compat_now();
	if (context->callDepth >= wrk->limits.max_recursion - 1 ||
		wrk->AVM1_cur_recursion_internal > AVM1_RECURSION_LIMIT_INTERN)
	{
		if (!context->exceptionthrown)
			wrk->throwStackOverflow();
		return;
	}
	Log::calls_indent++;
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" executeActions "<<preloadParent<<preloadRoot<<suppressSuper<<preloadSuper<<suppressArguments<<preloadArguments<<suppressThis<<preloadThis<<preloadGlobal<<" "<<startactionpos<<" "<<num_args);
	context->swfversion=clip->loadedFrom->version;
	context->callee = callee;

	if (!context->getGlobalScope())
	{
		auto global = clip->getSystemState()->avm1global;
		context->setGlobalScope(new AVM1Scope(global));
	}

	bool isClosure = context->swfversion >= 6;

	// NOTE: In SWF 5, function calls aren't closures, and always create
	// a new target scope, regardless of the function's origin.
	AVM1Scope* parentScope=nullptr;
	if (isClosure && scope)
	{
		scope->incRef();
		parentScope = scope;
	}
	else
	{
		context->getGlobalScope()->incRef();
		parentScope = new AVM1Scope
		(
			context->getGlobalScope(),
			clip
		);
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" setup new parent scope "<<asAtomHandler::toDebugString(parentScope->getLocals())<<" "<<parentScope);
	}

	// Local scopes are only created for function calls.
	if (scope)
	{
		auto sc = new AVM1Scope(parentScope, wrk);
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" setup new local scope "<<asAtomHandler::toDebugString(sc->getLocals()));
		context->setScope(sc);
		sc->getLocalsPtr()->decRef(); // remove ref received through new_asobject() in AVM1Scope constructor
	}
	else
		context->setScope(parentScope);

	wrk->AVM1callStack.push_back(context);
	context->callDepth++;
	if (result)
		asAtomHandler::setUndefined(*result);
	std::stack<asAtom> stack;
	asAtom registers[256];
	std::fill_n(registers,256,asAtomHandler::undefinedAtom);
	int curdepth = 0;
	int maxdepth = context->swfversion < 6 ? 8 : 16;
	auto thisObj = obj != nullptr ? *obj : asAtomHandler::fromObject(clip);
	std::vector<uint8_t>::const_iterator* scopestackstop = LS_STACKALLOC(std::vector<uint8_t>::const_iterator, maxdepth);
	scopestackstop[0] = actionlist.end();
	uint32_t currRegister = 1; // spec is not clear, but gnash starts at register 1
	if (!suppressThis || preloadThis)
	{
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" preload this:"<<asAtomHandler::toDebugString(thisObj));
		if (!suppressThis)
			ASATOM_ADDSTOREDMEMBER(thisObj);
		registers[currRegister++] = !suppressThis ? thisObj : asAtomHandler::undefinedAtom;
	}

	AVM1Array* argsArray = nullptr;
	if (!suppressArguments || preloadArguments)
	{
		argsArray = Class<AVM1Array>::getInstanceSNoArgs(wrk);
		argsArray->resize(num_args);
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" setup arguments array "<<argsArray->toDebugString()<<num_args);
		for (uint32_t i = 0; i < num_args; i++)
		{
			ASATOM_INCREF(args[i]);
			LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" parameter "<<i<<" "<<(paramnames.size() >= num_args ? clip->getSystemState()->getStringFromUniqueId(paramnames[i]):"")<<" "<<asAtomHandler::toDebugString(args[i]));
			argsArray->set(i,args[i],false,false);
			argsArray->addEnumerationValue(i,true);
		}

		auto proto = asAtomHandler::fromObject(argsArray->getClass()->prototype->getObj());
		argsArray->setprop_prototype(proto, BUILTIN_STRINGS::STRING_PROTO);

		nsNameAndKind tmpns(clip->getSystemState(), "", NAMESPACE);
		asAtom c = caller ? asAtomHandler::fromObject(caller) : asAtomHandler::nullAtom;

		if (caller)
			caller->incRef();

		argsArray->setVariableAtomByQName(BUILTIN_STRINGS::STRING_CALLER, tmpns, c, DYNAMIC_TRAIT, false, true, 5);

		if (callee)
		{
			callee->incRef();
			asAtom c = asAtomHandler::fromObject(callee);
			argsArray->setVariableAtomByQName(BUILTIN_STRINGS::STRING_CALLEE, tmpns, c, DYNAMIC_TRAIT, false, true, 5);
		}

		auto argsAtom = asAtomHandler::fromObject(argsArray);
		if (preloadArguments)
		{
			argsArray->addStoredMember();
			registers[currRegister++] = argsAtom;
		}
		else
		{
			if (context->getScope()->forceDefineLocal
			(
				BUILTIN_STRINGS::STRING_ARGUMENTS,
				argsAtom,
				CONST_ALLOWED,
				wrk
			))
				argsArray->decRef();
		}
	}
	asAtom super = asAtomHandler::invalidAtom;
	if (!suppressSuper || preloadSuper)
	{
		if (superobj && asAtomHandler::isObject(*superobj))
		{
			LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" super for "<<asAtomHandler::toDebugString(thisObj)<<" "<<asAtomHandler::toDebugString(*superobj));
			super.uintval=superobj->uintval;
		}
		if (asAtomHandler::isInvalid(super))
			LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" no super class found for "<<asAtomHandler::toDebugString(thisObj));
		if (preloadSuper)
		{
			// NOTE: The `super` register is set to `undefined`, if both
			// flags are set.
			if (!suppressSuper)
				ASATOM_ADDSTOREDMEMBER(super);
			registers[currRegister++] = (!suppressSuper) ? super : asAtomHandler::undefinedAtom;
		}
		else
		{
			ASATOM_INCREF(super);
			if (context->getScope()->forceDefineLocal
			(
				BUILTIN_STRINGS::STRING_SUPER,
				super,
				CONST_ALLOWED,
				wrk
			))
				ASATOM_DECREF(super);
		}
	}
	if (preloadRoot)
	{
		DisplayObject* root=clip->AVM1getRoot();
		root->incRef();
		root->addStoredMember();
		registers[currRegister++] = asAtomHandler::fromObject(root);
	}
	if (preloadParent)
	{
		// NOTE: If `_parent` is `undefined` (because it's a root clip),
		// it actually doesn't get pushed, with `_global` taking it's
		// place instead.
		if (clip->getParent() != nullptr)
		{
			clip->getParent()->incRef();
			clip->getParent()->addStoredMember();
			registers[currRegister++] = asAtomHandler::fromObject(clip->getParent());
		}
	}
	if (preloadGlobal)
	{
		clip->getSystemState()->avm1global->incRef();
		clip->getSystemState()->avm1global->addStoredMember();
		registers[currRegister++] = asAtomHandler::fromObject(clip->getSystemState()->avm1global);
	}
	for (uint32_t i = 0; i < paramnames.size() && i < num_args; i++)
	{
		auto reg = i < paramregisternumbers.size() ? paramregisternumbers[i] : 0;
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" set argument "<<i<<" "<<(int)reg<<" "<<clip->getSystemState()->getStringFromUniqueId(paramnames[i])<<" "<<asAtomHandler::toDebugString(args[i]));

		if (reg != 0)
		{
			ASATOM_ADDSTOREDMEMBER(args[i]);
			ASATOM_REMOVESTOREDMEMBER(registers[reg]);
			registers[reg] = args[i];
		}
		else
		{
			ASATOM_INCREF(args[i]);
			if (context->getScope()->forceDefineLocal
			(
				paramnames[i],
				args[i],
				CONST_ALLOWED,
				wrk
			))
				ASATOM_DECREF(args[i]);
		}
	}
	std::vector<tryCatchBlock> trycatchblocks;
	bool inCatchBlock = false;

	DisplayObject *originalclip = clip;
	auto it = actionlist.begin()+startactionpos;
	auto tryblockstart = actionlist.end();
	while (it != actionlist.end())
	{
		if (wrk->AVM1_cur_recursion_internal <= AVM1_RECURSION_LIMIT_INTERN && context->exceptionthrown && context->exceptionthrown->is<StackOverflowError>())
		{
			// stackoverflow exceptions on non-internal calls are never caught
			break;
		}
		if (!(context->actionsExecuted % 2000))
		{
			auto delta = compat_now() - context->startTime;
			if (delta.getSecs() >= wrk->limits.script_timeout)
			{
				std::stringstream s;
				s << "Reached maximum script execution time of " << wrk->limits.script_timeout << " seconds";
				throw ScriptLimitException
				(
					s.str(),
					ScriptLimitException::ScriptTimeout
				);
			}
		}
		if (tryblockstart!= actionlist.end())
		{
			tryCatchBlock trycatchblock = trycatchblocks.back();
			if (context->exceptionthrown)
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<< " "<<int(it-actionlist.begin())<< " exception caught: action code:"<<hex<<(int)*it<<" "<<dec<<context->exceptionthrown->toDebugString());
				it = tryblockstart + trycatchblock.trysize;
				if (!trycatchblock.name.empty())
				{
					uint32_t nameID = sys->getUniqueStringId
					(
						trycatchblock.name,
						context->isCaseSensitive()
					);
					auto e = asAtomHandler::fromObject(context->exceptionthrown);
					multiname m(nullptr);
					m.name_type = multiname::NAME_STRING;
					m.name_s_id = nameID;
					if (context->getScope()->setVariableByMultiname
						(
							m,
							e,
							CONST_ALLOWED,
							wrk
						))
						ASATOM_DECREF(e);

				}
				else if(trycatchblock.reg != UINT8_MAX)
				{
					ASATOM_REMOVESTOREDMEMBER(registers[trycatchblock.reg]);
					context->exceptionthrown->addStoredMember();
					registers[trycatchblock.reg] = asAtomHandler::fromObject(context->exceptionthrown);
				}
				else
					context->exceptionthrown->decRef();
				context->exceptionthrown=nullptr;
				inCatchBlock=true;
			}
			else if (it > tryblockstart + trycatchblock.trysize)
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<< " "<<int(it-actionlist.begin())<< " end of try block: action code:"<<hex<<(int)*it);
				if (!inCatchBlock && it < tryblockstart + trycatchblock.trysize + trycatchblock.catchsize)
				{
					it = tryblockstart + trycatchblock.trysize + trycatchblock.catchsize; // skip catchblock
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<< " "<<int(it-actionlist.begin())<< " skip catch block("<<trycatchblock.catchsize<<") action code:"<<hex<<(int)*it);
				}
				inCatchBlock=false;
				trycatchblocks.pop_back();
				if (!trycatchblocks.empty())
					tryblockstart = actionlist.begin()+ trycatchblocks.back().startpos;
				else
					tryblockstart = actionlist.end();
			}
			// TODO special handling for finally block necessary?
		}
		while (curdepth > 0 && it == scopestackstop[curdepth])
		{
			LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" end with "<<asAtomHandler::toDebugString(context->getScope()->getLocals()));
			AVM1Scope* sc = context->getScope()->getParentScope();
			if (sc)
			{
				sc->incRef();
				ASATOM_REMOVESTOREDMEMBER(sc->getLocals());
			}
			context->setScope(sc);
			curdepth--;
			Log::calls_indent--;
		}
		if (!clip
				&& *it != 0x20 // ActionSetTarget2
				&& *it != 0x8b // ActionSetTarget
				)
		{
			// we are in a target that was not found during ActionSetTarget(2), so these actions are ignored
			continue;
		}
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<< " "<<int(it-actionlist.begin())<<" stack:"<<stack.size()<<" action code:"<<hex<<(int)*it<<dec<<" "<<clip->toDebugString());
		uint8_t opcode = *it++;
		if (opcode > 0x80)
			it+=2; // skip length
		switch (opcode)
		{
			case 0x00:
				it = actionlist.end(); // force quit loop;
				break;
			case 0x04: // ActionNextFrame
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionNextFrame "<<clip->toDebugString());
					break;
				}
				if (invalidTarget)
				{
					break;
				}
				uint32_t frame = clip->as<MovieClip>()->state.FP+1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNextFrame "<<frame);
				clip->as<MovieClip>()->AVM1gotoFrame(frame,true,true);
				break;
			}
			case 0x05: // ActionPreviousFrame
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionPreviousFrame "<<clip->toDebugString());
					break;
				}
				if (invalidTarget)
				{
					break;
				}
				uint32_t frame = clip->as<MovieClip>()->state.FP-1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPreviousFrame "<<frame);
				clip->as<MovieClip>()->AVM1gotoFrame(frame,true,true);
				break;
			}
			case 0x06: // ActionPlay
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionPlay "<<clip->toDebugString());
					break;
				}
				if (invalidTarget)
				{
					break;
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPlay");
				clip->as<MovieClip>()->setPlaying();
				break;
			}
			case 0x07: // ActionStop
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionStop "<<clip->toDebugString());
					break;
				}
				if (invalidTarget)
				{
					break;
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStop");
				clip->as<MovieClip>()->setStopped();
				break;
			}
			case 0x09: // ActionStopSounds
			{
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStopSounds");
				clip->getSystemState()->audioManager->stopAllSounds();
				break;
			}
			case 0x0a: // ActionAdd
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				// we have to call AVM1toNumber for b first, as it may call an overridden valueOf()
				// so we call in the same order as adobe does
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionAdd "<<b<<"+"<<a);
				PushStack(stack,asAtomHandler::fromNumber(wrk,a+b,false));
				break;
			}
			case 0x0b: // ActionSubtract
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSubtract "<<b<<"-"<<a);
				PushStack(stack,asAtomHandler::fromNumber(wrk,b-a,false));
				break;
			}
			case 0x0c: // ActionMultiply
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				// we have to call AVM1toNumber for b first, as it may call an overridden valueOf()
				// so we call in the same order as adobe does
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionMultiply "<<b<<"*"<<a);
				PushStack(stack,asAtomHandler::fromNumber(wrk,b*a,false));
				break;
			}
			case 0x0d: // ActionDivide
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDivide "<<b<<"/"<<a);
				asAtom ret=asAtomHandler::invalidAtom;
				if (a == 0)
				{
					if (clip->loadedFrom->version == 4)
						ret = asAtomHandler::fromString(clip->getSystemState(),"#ERROR#");
					else
						ret = asAtomHandler::fromNumber(wrk,Number::NaN,false);
				}
				else
					ret = asAtomHandler::fromNumber(wrk,b/a,false);
				PushStack(stack,ret);
				break;
			}
			case 0x0e: // ActionEquals
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionEquals "<<b<<"=="<<a<<" "<<(b==a));
				if (wrk->AVM1getSwfVersion() < 5)
					PushStack(stack,asAtomHandler::fromInt(b==a ? 1 : 0));
				else
					PushStack(stack,asAtomHandler::fromBool(b==a));
				break;
			}
			case 0x0f: // ActionLess
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				// we have to call AVM1toNumber for b first, as it may call an overridden valueOf()
				// so we call in the same order as adobe does
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionLess "<<b<<"<"<<a);
				if (wrk->AVM1getSwfVersion() < 5)
					PushStack(stack,asAtomHandler::fromInt(b<a ? 1 : 0));
				else
					PushStack(stack,asAtomHandler::fromBool(b<a));
				break;
			}
			case 0x10: // ActionAnd
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				bool a = asAtomHandler::AVM1toBool(aa,wrk,clip->loadedFrom->version);
				bool b = asAtomHandler::AVM1toBool(ab,wrk,clip->loadedFrom->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionAnd "<<asAtomHandler::toDebugString(aa)<<" && "<<asAtomHandler::toDebugString(ab));
				PushStack(stack,asAtomHandler::fromBool(b && a));
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				break;
			}
			case 0x11: // ActionOr
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				bool a = asAtomHandler::AVM1toBool(aa,wrk,clip->loadedFrom->version);
				bool b = asAtomHandler::AVM1toBool(ab,wrk,clip->loadedFrom->version);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionOr "<<asAtomHandler::toDebugString(aa)<<" || "<<asAtomHandler::toDebugString(ab));
				PushStack(stack,asAtomHandler::fromBool(b || a));
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				break;
			}
			case 0x12: // ActionNot
			{
				asAtom a = PopStack(stack);
				bool value = asAtomHandler::AVM1toBool(a,wrk,clip->loadedFrom->version);
				ASATOM_DECREF(a);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNot "<<value);
				PushStack(stack,asAtomHandler::fromBool(!value));
				break;
			}
			case 0x13: // ActionStringEquals
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				tiny_string a = asAtomHandler::AVM1toString(aa,wrk);
				tiny_string b = asAtomHandler::AVM1toString(ab,wrk);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStringEquals "<<a<<" "<<b);
				if (wrk->AVM1getSwfVersion() < 5)
					PushStack(stack,asAtomHandler::fromInt(b == a ? 1 : 0));
				else
					PushStack(stack,asAtomHandler::fromBool(b == a));
				break;
			}
			case 0x15: // ActionStringExtract
			{
				asAtom count = PopStack(stack);
				asAtom index = PopStack(stack);
				asAtom str = PopStack(stack);
				tiny_string a = asAtomHandler::AVM1toString(str,wrk);
				// start parameter seems to be 1-based
				int startindex = asAtomHandler::toInt(index)-1;
				tiny_string b;
				if (uint32_t(startindex) <= a.numChars())
					b = a.substr(startindex,asAtomHandler::toInt(count));
				ASATOM_DECREF(count);
				ASATOM_DECREF(index);
				ASATOM_DECREF(str);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStringExtract "<<a<<" "<<b);
				PushStack(stack,asAtomHandler::fromString(clip->getSystemState(),b));
				break;
			}
			case 0x17: // ActionPop
			{
				asAtom a= PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPop "<<asAtomHandler::toDebugString(a));
				ASATOM_DECREF(a);
				break;
			}
			case 0x18: // ActionToInteger
			{
				asAtom a = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionToInteger "<<asAtomHandler::toDebugString(a));
				PushStack(stack,asAtomHandler::fromInt(asAtomHandler::toInt(a)));
				ASATOM_DECREF(a);
				break;
			}
			case 0x1c: // ActionGetVariable
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetVariable "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(thisObj));
				auto s = asAtomHandler::AVM1toString(name, wrk);
				asAtom res = asAtomHandler::invalidAtom;
				if (s == "super")
					res = new_AVM1SuperObject(thisObj,callee->getAVM1Class(),wrk);
				else
					res = context->getVariable(thisObj, originalclip, clip, s);

				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetVariable done "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(res));
				ASATOM_DECREF(name);
				PushStack(stack,res);
				break;
			}
			case 0x1d: // ActionSetVariable
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetVariable "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(value));
				auto s = asAtomHandler::AVM1toString(name, wrk);
				ASATOM_DECREF(name);
				context->setVariable(thisObj, originalclip, clip, s, value);
				break;
			}
			case 0x20: // ActionSetTarget2
			{
				asAtom a = PopStack(stack);
				if (asAtomHandler::is<DisplayObject>(a))
				{
					if (clip_isTarget)
						clip->decRef();
					clip = asAtomHandler::as<MovieClip>(a);
					clip_isTarget=true;
					LOG_CALL("AVM1: ActionSetTarget2: setting target to "<<clip->toDebugString());
				}
				else
				{
					tiny_string s = asAtomHandler::toString(a,wrk);
					ASATOM_DECREF(a);
					if (!clip)
					{
						LOG_CALL("AVM1: ActionSetTarget2: setting target from undefined value to "<<s);
					}
					else
					{
						LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetTarget2 "<<s);
					}

					if (clip_isTarget)
						clip->decRef();
					clip_isTarget=false;
					if (s.empty())
					{
						clip = originalclip;
					}
					else
					{
						DisplayObject* c = clip->AVM1GetClipFromPath(s);
						clip = c != nullptr ? c : clip->AVM1getRoot();
						if (c == nullptr)
							LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetTarget2 clip not found:"<<s);
						clip->incRef();
						clip_isTarget=true;
					}
				}
				context->getScope()->setTargetScope(clip);
				break;
			}
			case 0x21: // ActionStringAdd
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				tiny_string a = asAtomHandler::toString(aa,wrk);
				tiny_string b = asAtomHandler::toString(ab,wrk);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStringAdd "<<b<<"+"<<a);
				asAtom ret = asAtomHandler::fromString(clip->getSystemState(),b+a);
				PushStack(stack,ret);
				break;
			}
			case 0x22: // ActionGetProperty
			{
				asAtom index = PopStack(stack);
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetProperty "<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(index));
				DisplayObject* o = clip;
				if (asAtomHandler::toStringId(target,wrk) != BUILTIN_STRINGS::EMPTY)
				{
					tiny_string s = asAtomHandler::toString(target,wrk);
					o = clip->AVM1GetClipFromPath(s);
				}
				asAtom ret = asAtomHandler::invalidAtom;
				if (o != nullptr)
				{
					size_t idx = asAtomHandler::toInt(index);
					ret = o->getPropertyByIndex(idx, wrk);
				}
				if (asAtomHandler::isInvalid(ret))
					ret = asAtomHandler::undefinedAtom;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetProperty done "<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(ret));
				ASATOM_DECREF(index);
				ASATOM_DECREF(target);
				PushStack(stack,ret);
				break;
			}
			case 0x23: // ActionSetProperty
			{
				asAtom value = PopStack(stack);
				asAtom index = PopStack(stack);
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetProperty "<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(index)<<" "<<asAtomHandler::toDebugString(value));
				DisplayObject* o = clip;
				if (asAtomHandler::is<MovieClip>(target))
				{
					o = asAtomHandler::as<MovieClip>(target);
				}
				else if (asAtomHandler::toStringId(target,wrk) != BUILTIN_STRINGS::EMPTY)
				{
					tiny_string s = asAtomHandler::toString(target,wrk);
					o = clip->AVM1GetClipFromPath(s);
					if (!o) // it seems that Adobe falls back to the current clip if the path is invalid
						o = clip;
				}
				if (o != nullptr)
				{
					size_t idx = asAtomHandler::toInt(index);
					o->setPropertyByIndex(idx, value, wrk);
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetProperty target clip not found:"<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(index)<<" "<<asAtomHandler::toDebugString(value));
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetProperty done:"<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(index)<<" "<<asAtomHandler::toDebugString(value));
				ASATOM_DECREF(index);
				ASATOM_DECREF(target);
				break;
			}
			case 0x24: // ActionCloneSprite
			{
				asAtom ad = PopStack(stack);
				uint32_t depth = asAtomHandler::toUInt(ad);
				asAtom target = PopStack(stack);
				asAtom sp = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCloneSprite "<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(sp)<<" "<<depth);
				DisplayObject* source = asAtomHandler::is<DisplayObject>(sp) ? asAtomHandler::as<DisplayObject>(sp) : nullptr;
				if (!source)
				{
					tiny_string sourcepath = asAtomHandler::toString(sp,wrk);
					source = clip->AVM1GetClipFromPath(sourcepath);
				}
				if (source)
				{
					DisplayObjectContainer* parent = source->getParent();
					if (!parent || !parent->is<MovieClip>())
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCloneSprite: the clip has no parent:"<<asAtomHandler::toDebugString(target)<<" "<<source->toDebugString()<<" "<<depth);
					else if (!source->is<MovieClip>())
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCloneSprite: the source is not a MovieClip:"<<asAtomHandler::toDebugString(target)<<" "<<source->toDebugString()<<" "<<depth);
					else
						source->as<MovieClip>()->AVM1CloneSprite(target,depth,nullptr);
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCloneSprite source clip not found:"<<asAtomHandler::toDebugString(target)<<" "<<asAtomHandler::toDebugString(sp)<<" "<<depth);
				ASATOM_DECREF(ad);
				ASATOM_DECREF(target);
				ASATOM_DECREF(sp);
				break;
			}
			case 0x25: // ActionRemoveSprite
			{
				asAtom target = PopStack(stack);
				DisplayObject* o = nullptr;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionRemoveSprite "<<asAtomHandler::toDebugString(target));
				if (asAtomHandler::is<DisplayObject>(target))
				{
					o = asAtomHandler::as<DisplayObject>(target);
				}
				else
				{
					uint32_t targetID = asAtomHandler::toStringId(target,wrk);
					if (targetID != BUILTIN_STRINGS::EMPTY)
					{
						tiny_string s = asAtomHandler::toString(target,wrk);
						o = clip->AVM1GetClipFromPath(s);
					}
					else
						o = clip;
				}
				if (o && o->getParent() && !o->is<RootMovieClip>() && !o->legacy)
				{
					asAtom obj = asAtomHandler::fromObjectNoPrimitive(o);
					asAtom r = asAtomHandler::invalidAtom;
					MovieClip::AVM1RemoveMovieClip(r,wrk,obj,nullptr,0);
				}
				ASATOM_DECREF(target);
				break;
			}
			case 0x26: // ActionTrace
			{
				asAtom value = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionTrace "<<asAtomHandler::toDebugString(value));
				clip->getSystemState()->trace(asAtomHandler::AVM1toString(value,wrk,true));
				ASATOM_DECREF(value);
				break;
			}
			case 0x27: // ActionStartDrag
			{
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStartDrag "<<asAtomHandler::toDebugString(target));
				asAtom lockcenter = PopStack(stack);
				asAtom constrain = PopStack(stack);
				asAtom obj=asAtomHandler::invalidAtom;
				if (asAtomHandler::is<MovieClip>(target))
				{
					obj = target;
				}
				else
				{
					tiny_string s = asAtomHandler::AVM1toString(target,wrk);
					ASATOM_DECREF(target);
					DisplayObject* targetclip = clip->AVM1GetClipFromPath(s);
					if (targetclip)
					{
						targetclip->incRef();
						obj = asAtomHandler::fromObject(targetclip);
					}
					else
					{
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStartDrag target clip not found:"<<asAtomHandler::toDebugString(target));
					}
				}
				asAtom args[5];
				bool islockcenter = asAtomHandler::AVM1toNumber(lockcenter,clip->loadedFrom->version);
				args[0] = asAtomHandler::fromBool(islockcenter);
				bool hasconstraints = asAtomHandler::AVM1toNumber(constrain,clip->loadedFrom->version);
				if (hasconstraints)
				{
					args[4] = PopStack(stack);
					args[3] = PopStack(stack);
					args[2] = PopStack(stack);
					args[1] = PopStack(stack);
				}
				asAtom ret=asAtomHandler::invalidAtom;
				AVM1MovieClip::startDrag(ret,wrk,obj,args,hasconstraints ? 5 : 1);
				if (hasconstraints)
				{
					ASATOM_DECREF(args[1]);
					ASATOM_DECREF(args[2]);
					ASATOM_DECREF(args[3]);
					ASATOM_DECREF(args[4]);
				}
				ASATOM_DECREF(obj);
				ASATOM_DECREF(lockcenter);
				ASATOM_DECREF(constrain);
				break;
			}
			case 0x28: // ActionEndDrag
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(clip);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionEndDrag "<<asAtomHandler::toDebugString(obj));
				AVM1MovieClip::stopDrag(ret,wrk,obj,nullptr,0);
				break;
			}
			case 0x2a: // ActionThrow
			{
				asAtom obj = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionThrow "<<asAtomHandler::toDebugString(obj));
				if (!context->exceptionthrown)
					context->exceptionthrown=asAtomHandler::toObject(obj,wrk);
				else
				{
					ASATOM_DECREF(obj);
				}
				if (tryblockstart == actionlist.end())
				{
					it = actionlist.end(); // force quit loop;
				}
				break;
			}
			case 0x2b: // ActionCastOp
			{
				asAtom obj = PopStack(stack);
				asAtom constr = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCastOp "<<asAtomHandler::toDebugString(obj)<<" "<<asAtomHandler::toDebugString(constr));
				ASObject* value = asAtomHandler::toObject(obj,wrk);
				ASObject* type = asAtomHandler::toObject(constr,wrk);
				if (type == Class<ASObject>::getRef(clip->getSystemState()).getPtr() || implementsInterface(constr,value,wrk))
				{
					PushStack(stack,obj);
				}
				else
				{
					ASATOM_DECREF(obj);
					PushStack(stack,asAtomHandler::nullAtom);
				}
				ASATOM_DECREF(constr);
				break;
			}
			case 0x69: // ActionExtends
			{
				asAtom superconstr = PopStack(stack);
				asAtom subconstr = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionExtends "<<asAtomHandler::toDebugString(superconstr)<<" "<<asAtomHandler::toDebugString(subconstr));
				if (!asAtomHandler::isFunction(subconstr))
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionExtends  wrong constructor type "<<asAtomHandler::toDebugString(superconstr)<<" "<<asAtomHandler::toDebugString(subconstr));
					break;
				}
				IFunction* c = asAtomHandler::as<IFunction>(subconstr);
				c->setRefConstant();
				ASObject* pr = new_asobject(wrk);
				pr->addStoredMember();
				ASATOM_REMOVESTOREDMEMBER(c->prototype);
				c->prototype = asAtomHandler::fromObject(pr);
				c->setprop_prototype(c->prototype);
				c->setprop_prototype(c->prototype,BUILTIN_STRINGS::STRING_PROTO);
				ASObject* superobj = asAtomHandler::toObject(superconstr,wrk);
				ASObject* proto = superobj->getprop_prototype();
				if (!proto)
				{
					if (superobj->is<Class_base>())
						proto = superobj->as<Class_base>()->getPrototype(wrk)->getObj();
					else if (superobj->is<IFunction>())
						proto = asAtomHandler::getObject(superobj->as<IFunction>()->prototype);
				}
				if (proto)
				{
					asAtom o = asAtomHandler::fromObject(proto);
					pr->setprop_prototype(o);
					pr->setprop_prototype(o,BUILTIN_STRINGS::STRING_PROTO);
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionExtends  super prototype not found "<<asAtomHandler::toDebugString(superconstr)<<" "<<asAtomHandler::toDebugString(subconstr));
				pr->setVariableAtomByQName("constructor",nsNameAndKind(),superconstr, DECLARED_TRAIT,false);
				if (c->is<AVM1Function>())
				{
					c->as<AVM1Function>()->setAVM1Class(pr);
					c->as<AVM1Function>()->setSuper(superconstr);
				}
				break;
			}
			case 0x30: // ActionRandomNumber
			{
				asAtom am = PopStack(stack);
				int maximum = asAtomHandler::toInt(am);
				int ret = (((number_t)rand())/(number_t)RAND_MAX)*maximum;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionRandomNumber "<<ret);
				ASATOM_DECREF(am);
				PushStack(stack,asAtomHandler::fromInt(ret));
				break;
			}
			case 0x34: // ActionGetTime
			{
				uint64_t runtime = clip->getSystemState()->getCurrentTime_ms()-clip->getSystemState()->startTime;
				asAtom ret=asAtomHandler::fromNumber(wrk,(number_t)runtime,false);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetTime "<<asAtomHandler::toDebugString(ret));
				PushStack(stack,asAtom(ret));
				break;
			}
			case 0x3a: // ActionDelete
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDelete "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
				asAtom ret = asAtomHandler::falseAtom;
				if (asAtomHandler::isObject(scriptobject))
				{
					ASObject* o = asAtomHandler::getObject(scriptobject);
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id = sys->getUniqueStringId
					(
						asAtomHandler::AVM1toString(name, wrk),
						context->isCaseSensitive()
					);
					m.ns.emplace_back(clip->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);
					m.isAttribute = false;
					if (o->deleteVariableByMultiname(m,wrk))
						ret = asAtomHandler::trueAtom;
				}
				else
					LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDelete for scriptobject type "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
				ASATOM_DECREF(name);
				ASATOM_DECREF(scriptobject);
				PushStack(stack,asAtom(ret)); // spec doesn't mention this, but it seems that a bool indicating wether a property was deleted is pushed on the stack
				break;
			}
			case 0x3b: // ActionDelete2
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDelete2 "<<asAtomHandler::toDebugString(name));
				multiname m(nullptr);
				m.name_type = multiname::NAME_STRING;
				m.name_s_id = sys->getUniqueStringId
				(
					asAtomHandler::AVM1toString(name, wrk),
					context->isCaseSensitive()
				);
				m.ns.emplace_back(clip->getSystemState(),BUILTIN_STRINGS::EMPTY,NAMESPACE);

				bool ret = context->getScope()->deleteVariableByMultiname(m, wrk);

				ASATOM_DECREF(name);
				PushStack(stack,asAtomHandler::fromBool(ret)); // spec doesn't mention this, but it seems that a bool indicating wether a property was deleted is pushed on the stack
				break;
			}
			case 0x3c: // ActionDefineLocal
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineLocal "<<asAtomHandler::toDebugString(name)<<" " <<asAtomHandler::toDebugString(value));

				multiname m(nullptr);
				m.name_type = multiname::NAME_STRING;
				m.name_s_id = sys->getUniqueStringId
				(
					asAtomHandler::AVM1toString
					(
						name,
						wrk
					),
					context->isCaseSensitive()
				);
				if (context->getScope()->defineLocalByMultiname
				(
					m,
					value,
					CONST_ALLOWED,
					wrk
				))
					ASATOM_DECREF(value);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineLocal done "<<asAtomHandler::toDebugString(name));
				ASATOM_DECREF(name);
				break;
			}
			case 0x3d: // ActionCallFunction
			{
				asAtom name = PopStack(stack);
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				asAtom* args = numargs ? LS_STACKALLOC(asAtom, numargs) : nullptr;
				for (uint32_t i = 0; i < numargs; i++)
				{
					args[i] = PopStack(stack);
					ASObject* a = asAtomHandler::getObject(args[i]);
					if (a)
						a->addStoredMember();
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(thisObj));

				asAtom ret = asAtomHandler::undefinedAtom;
				auto s = asAtomHandler::AVM1toString(name, wrk);
				asAtom func = asAtomHandler::invalidAtom;
				if (callee && s== "super")
				{
					func = callee->getSuper();
					ASATOM_INCREF(func);
				}
				else
					func = context->getVariable(thisObj, originalclip, clip, s);

				if (asAtomHandler::is<AVM1Function>(func))
				{
					auto f = asAtomHandler::as<AVM1Function>(func);
					f->call(&ret,&thisObj,args,numargs,caller);
					f->decRef();
				}
				else if (asAtomHandler::is<Function>(func))
				{
					asAtomHandler::as<Function>(func)->call(ret,wrk,thisObj,args,numargs);
					asAtomHandler::as<Function>(func)->decRef();
				}
				else if (asAtomHandler::is<Class_base>(func))
					asAtomHandler::as<Class_base>(func)->generator(wrk,ret,args,numargs);
				else
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction function not found "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func)<<" "<<numargs);

				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction done "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				for (uint32_t i = 0; i < numargs; i++)
					ASATOM_REMOVESTOREDMEMBER(args[i]);
				ASATOM_DECREF(name);
				ASATOM_DECREF(na);
				PushStack(stack,ret);
				break;
			}
			case 0x3e: // ActionReturn
			{
				if (result)
					*result = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionReturn ");
				it = actionlist.end();
				break;
			}
			case 0x3f: // ActionModulo
			{
				// spec is wrong, y is popped of stack first
				asAtom y = PopStack(stack);
				asAtom x = PopStack(stack);
				asAtom tmp = x;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionModulo "<<asAtomHandler::toDebugString(x)<<" % "<<asAtomHandler::toDebugString(y));
				asAtomHandler::modulo(x,wrk,y,false);
				ASATOM_DECREF(y);
				ASATOM_DECREF(tmp);
				PushStack(stack,x);
				break;
			}
			case 0x40: // ActionNewObject
			{
				asAtom name = PopStack(stack);
				asAtom na = PopStack(stack);
				size_t numargs = std::min((size_t)asAtomHandler::toUInt(na), stack.size());
				asAtom* args = LS_STACKALLOC(asAtom, numargs);
				for (size_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewObject "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				auto s = asAtomHandler::AVM1toString(name, wrk);
				auto nameID = sys->getUniqueStringId(s, context->isCaseSensitive());
				asAtom ret=asAtomHandler::invalidAtom;
				if (asAtomHandler::isUndefined(name) || s.empty())
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewObject without name "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				else
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;

					auto cls = context->getScope()->getVariableByMultiname
					(
						originalclip,
						m,
						GET_VARIABLE_OPTION::NONE,
						wrk
					);

					if (asAtomHandler::is<Class_base>(cls))
					{
						asAtomHandler::as<Class_base>(cls)->getInstance(wrk,ret,true,args,numargs);
					}
					else if (asAtomHandler::is<AVM1Function>(cls))
					{
						AVM1Function* f = asAtomHandler::as<AVM1Function>(cls);
						ASObject* baseclass = asAtomHandler::getObject(f->getSuper());
						if (baseclass && baseclass->is<Class_base>())
							baseclass->as<Class_base>()->getInstance(wrk,ret,true,nullptr,0);
						else
							ret = asAtomHandler::fromObjectNoPrimitive(new_asobject(wrk));

						ASObject* o = asAtomHandler::getObjectNoCheck(ret);
						o->setprop_prototype(f->prototype);
						o->setprop_prototype(f->prototype,BUILTIN_STRINGS::STRING_PROTO);
						if (wrk->AVM1getSwfVersion() < 7)
						{
							ASObject* pr = asAtomHandler::getObject(f->prototype);
							if (pr)
								f->setAVM1Class(pr);
							f->incRef();
							o->setVariableByQName("constructor","",f,DYNAMIC_TRAIT,false);
						}
						ret = asAtomHandler::fromObject(o);
						f->call(nullptr,&ret, args,numargs,caller);
						f->decRef();
						for (size_t i = 0; i < numargs; i++)
							ASATOM_DECREF(args[i]);
					}
				}
				if (asAtomHandler::isInvalid(ret))
				{
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewObject class not found "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
					for (size_t i = 0; i < numargs; i++)
						ASATOM_DECREF(args[i]);
				}
				ASATOM_DECREF(name);
				ASATOM_DECREF(na);
				PushStack(stack,ret);
				break;
			}

			case 0x41: // ActionDefineLocal2
			{
				asAtom name = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineLocal2 "<<asAtomHandler::toDebugString(name));
				tiny_string s = asAtomHandler::AVM1toString(name, wrk);
				uint32_t nameID = sys->getUniqueStringId(s, context->isCaseSensitive());
				multiname m(nullptr);
				m.name_type = multiname::NAME_STRING;
				m.name_s_id = nameID;

				if (!asAtomHandler::hasPropertyByMultiname(context->getScope()->getLocals(),m, true, false, wrk))
				{
					auto atom = asAtomHandler::undefinedAtom;
					context->getScope()->defineLocalByMultiname(m, atom, CONST_ALLOWED, wrk);
				}
				ASATOM_DECREF(name);
				break;
			}
			case 0x42: // ActionInitArray
			{
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionInitArray "<<numargs);
				AVM1Array* ret=Class<AVM1Array>::getInstanceSNoArgs(wrk);
				ret->resize(numargs);
				for (uint32_t i = 0; i < numargs; i++)
				{
					asAtom value = PopStack(stack);
					ret->set(i,value,false,false);
					ret->addEnumerationValue(i,true);
				}
				ASATOM_DECREF(na);
				PushStack(stack,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x43: // ActionInitObject
			{
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionInitObject "<<numargs);
				ASObject* ret=new_asobject(wrk);
				//Duplicated keys overwrite the previous value
				for (uint32_t i = 0; i < numargs; i++)
				{
					asAtom value = PopStack(stack);
					asAtom name = PopStack(stack);
					uint32_t nameid=asAtomHandler::toStringId(name,wrk);
					ret->setDynamicVariableNoCheck(nameid,value,asAtomHandler::isInteger(name),wrk);
				}
				ASATOM_DECREF(na);
				PushStack(stack,asAtomHandler::fromObject(ret));
				break;
			}
			case 0x44: // ActionTypeOf
			{
				asAtom obj = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionTypeOf "<<asAtomHandler::toDebugString(obj));
				asAtom res=asAtomHandler::invalidAtom;
				if (asAtomHandler::is<MovieClip>(obj))
					res = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_MOVIECLIP);
				else if (asAtomHandler::is<Date>(obj))
					res = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_STRING);
				else if (asAtomHandler::is<Class_base>(obj))
					res = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_FUNCTION_LOWERCASE);
				else
					res = asAtomHandler::typeOf(obj);
				ASATOM_DECREF(obj);
				PushStack(stack,res);
				break;
			}
			case 0x46: // ActionEnumerate
			{
				asAtom path = PopStack(stack);
				tiny_string s = asAtomHandler::AVM1toString(path, wrk);
				asAtom obj = context->getVariable(thisObj, originalclip, clip, s);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionEnumerate "<<s<<" "<<asAtomHandler::toDebugString(obj));
				if (asAtomHandler::isObject(obj) && !asAtomHandler::isNumeric(obj))
				{
					ACTIONRECORD::PushStack(stack,asAtomHandler::undefinedAtom);
					ASObject* o = asAtomHandler::toObject(obj,wrk);
					o->AVM1enumerate(stack);
				}
				else
					PushStack(stack,asAtomHandler::undefinedAtom);
				ASATOM_DECREF(obj);
				ASATOM_DECREF(path);
				break;
			}
			case 0x47: // ActionAdd2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				ASObject* o = asAtomHandler::getObject(arg2);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionAdd2 "<<asAtomHandler::toDebugString(arg2)<<" + "<<asAtomHandler::toDebugString(arg1));
				if (asAtomHandler::AVM1add(arg2,arg1,wrk,false))
				{
					if (o)
						o->decRef();
				}
				ASATOM_DECREF(arg1);
				PushStack(stack,arg2);
				break;
			}
			case 0x48: // ActionLess2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionLess2 "<<asAtomHandler::toDebugString(arg2)<<" < "<<asAtomHandler::toDebugString(arg1));
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::AVM1isLess(arg2,wrk,arg1) == TTRUE));
				ASATOM_DECREF(arg1);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x49: // ActionEquals2
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionEquals2 "<<asAtomHandler::toDebugString(arg2)<<" == "<<asAtomHandler::toDebugString(arg1));
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::AVM1isEqual(arg2, arg1, wrk)));
				ASATOM_DECREF(arg1);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x4a: // ActionToNumber
			{
				asAtom arg = PopStack(stack);
				PushStack(stack,asAtomHandler::fromNumber(wrk,asAtomHandler::AVM1toNumber(arg,clip->loadedFrom->version),false));
				ASATOM_DECREF(arg);
				break;
			}
			case 0x4b: // ActionToString
			{
				asAtom arg = PopStack(stack);
				PushStack(stack,asAtomHandler::fromString(clip->getSystemState(),asAtomHandler::toString(arg,wrk)));
				ASATOM_DECREF(arg);
				break;
			}
			case 0x4c: // ActionPushDuplicate
			{
				asAtom a = PeekStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPushDuplicate "<<asAtomHandler::toDebugString(a));
				ASATOM_INCREF(a);
				PushStack(stack,a);
				break;
			}
			case 0x4d: // ActionStackSwap
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStackSwap "<<asAtomHandler::toDebugString(arg1)<<" "<<asAtomHandler::toDebugString(arg2));
				PushStack(stack,arg1);
				PushStack(stack,arg2);
				break;
			}
			case 0x4e: // ActionGetMember
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				asAtom ret=asAtomHandler::invalidAtom;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
				if (asAtomHandler::isNull(scriptobject) || asAtomHandler::isUndefined(scriptobject) || asAtomHandler::isInvalid(scriptobject))
				{
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember called on Null/Undefined, ignored "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
					asAtomHandler::setUndefined(ret);
				}
				else
				{
					auto s = asAtomHandler::AVM1toString(name, wrk);
					auto nameID = sys->getUniqueStringId(s, context->isCaseSensitive());
					ASObject* o = asAtomHandler::toObject(scriptobject,wrk);
					multiname m(nullptr);
					m.isAttribute = false;
					switch (asAtomHandler::getObjectType(name))
					{
						case T_INTEGER:
							m.name_type=multiname::NAME_INT;
							m.name_i=asAtomHandler::getInt(name);
							break;
						case T_UINTEGER:
							m.name_type=multiname::NAME_UINT;
							m.name_ui=asAtomHandler::getUInt(name);
							break;
						case T_NUMBER:
							m.name_type=multiname::NAME_NUMBER;
							m.name_d=asAtomHandler::toNumber(name);
							break;
						default:
							m.name_type=multiname::NAME_STRING;
							m.name_s_id=nameID;
							break;
					}
					if (o != nullptr)
					{
						o->AVM1getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::DONT_CHECK_CLASS,wrk,false);
						if (asAtomHandler::isInvalid(ret))
							o->getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::NONE,wrk);
					}
					if (asAtomHandler::isInvalid(ret))
					{
						switch (asAtomHandler::getObjectType(scriptobject))
						{
							case T_FUNCTION:
								if (nameID == BUILTIN_STRINGS::PROTOTYPE)
								{
									ret = o->as<IFunction>()->prototype;
									ASATOM_INCREF(ret);
								}
								break;
							case T_OBJECT:
							case T_ARRAY:
							case T_CLASS:
								switch (nameID)
								{
									case BUILTIN_STRINGS::PROTOTYPE:
									{
										ASObject* p = o->AVM1getClassPrototypeObject();
										if (p)
										{
											p->incRef();
											ret = asAtomHandler::fromObject(p);
										}
										break;
									}
									default:
									{
										o->AVM1getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::NONE,wrk,false);
										break;
									}
								}
								break;
							default:
								LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember for scriptobject type "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
								break;
						}
					}
					if (context->exceptionthrown && !context->exceptionthrown->is<StackOverflowError>())
					{
						context->exceptionthrown->decRef();
						context->exceptionthrown=nullptr;
					}
					if (asAtomHandler::isInvalid(ret))
						asAtomHandler::setUndefined(ret);
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember done "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name)<<" " <<asAtomHandler::toDebugString(ret));
				ASATOM_DECREF(name);
				ASATOM_DECREF(scriptobject);
				PushStack(stack,ret);
				break;
			}
			case 0x4f: // ActionSetMember
			{
				asAtom value = PopStack(stack);
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetMember "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(value));
				if (asAtomHandler::isObject(scriptobject) || asAtomHandler::isFunction(scriptobject) || asAtomHandler::isArray(scriptobject))
				{
					auto s = asAtomHandler::AVM1toString(name, wrk);
					auto nameID = sys->getUniqueStringId(s, context->isCaseSensitive());
					ASObject* o = asAtomHandler::getObject(scriptobject);
					multiname m(nullptr);
					switch (asAtomHandler::getObjectType(name))
					{
						case T_INTEGER:
							m.name_type=multiname::NAME_INT;
							m.name_i=asAtomHandler::getInt(name);
							break;
						case T_UINTEGER:
							m.name_type=multiname::NAME_UINT;
							m.name_ui=asAtomHandler::getUInt(name);
							break;
						case T_NUMBER:
							m.name_type=multiname::NAME_NUMBER;
							m.name_d=asAtomHandler::toNumber(name);
							break;
						default:
							m.name_type=multiname::NAME_STRING;
							m.name_s_id=nameID;
							break;
					}
					m.isAttribute = false;
					if (m.name_type==multiname::NAME_STRING && m.name_s_id == BUILTIN_STRINGS::PROTOTYPE)
					{
						if (o->is<AVM1Function>())
						{
							AVM1Function* f = o->as<AVM1Function>();
							ASATOM_REMOVESTOREDMEMBER(f->prototype);
							f->prototype = value;
							ASATOM_ADDSTOREDMEMBER(f->prototype);
						}
						o->setprop_prototype(value);
						o->setprop_prototype(value,BUILTIN_STRINGS::STRING_PROTO);
						ASATOM_DECREF(value);
					}
					else
					{
						if (asAtomHandler::is<AVM1Function>(value))
						{
							AVM1Function* f = asAtomHandler::as<AVM1Function>(value);
							if (!o->is<Global>())
								f->setAVM1Class(o);
							if (f->needsSuper() && o->is<AVM1Function>())
							{
								asAtom a =f->getSuper();
								if (asAtomHandler::isInvalid(a))
								{
									ASObject* sup = o->getprop_prototype();
									if (!sup && o->getClass() != Class<ASObject>::getRef(wrk->getSystemState()).getPtr())
										sup = o->getClass();
									if (sup)
										f->setSuper(asAtomHandler::fromObjectNoPrimitive(sup));
								}
							}
						}
						if (o->AVM1setVariableByMultiname(m,value,CONST_ALLOWED,wrk))
							ASATOM_DECREF(value);
					}
				}
				else
				{
					if (!asAtomHandler::isUndefined(scriptobject))
						LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetMember for scriptobject type "<<asAtomHandler::toDebugString(scriptobject)<<" "<<(int)asAtomHandler::getObjectType(scriptobject)<<" "<<asAtomHandler::toDebugString(name));
					ASATOM_DECREF(value);
				}
				ASATOM_DECREF(name);
				ASATOM_DECREF(scriptobject);
				break;
			}
			case 0x50: // ActionIncrement
			{
				asAtom num = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionIncrement "<<asAtomHandler::toDebugString(num));
				PushStack(stack,asAtomHandler::fromNumber(wrk,asAtomHandler::AVM1toNumber(num,clip->loadedFrom->version)+1,false));
				ASATOM_DECREF(num);
				break;
			}
			case 0x51: // ActionDecrement
			{
				asAtom num = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDecrement "<<asAtomHandler::toDebugString(num));
				PushStack(stack,asAtomHandler::fromNumber(wrk,asAtomHandler::AVM1toNumber(num,clip->loadedFrom->version)-1,false));
				ASATOM_DECREF(num);
				break;
			}
			case 0x52: // ActionCallMethod
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				asAtom* args = numargs ? LS_STACKALLOC(asAtom, numargs) : nullptr;
				for (uint32_t i = 0; i < numargs; i++)
				{
					args[i] = PopStack(stack);
					ASObject* a = asAtomHandler::getObject(args[i]);
					if (a)
						a->addStoredMember();
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
				asAtom ret=asAtomHandler::invalidAtom;
				bool done=false;
				tiny_string s;
				if (asAtomHandler::isValid(name) && !asAtomHandler::isUndefined(name))
					s = asAtomHandler::AVM1toString(name, wrk);
				auto nameID = sys->getUniqueStringId(s, context->isCaseSensitive());
				if (s.empty())
				{
					if (asAtomHandler::is<Function>(scriptobject))
					{
						asAtomHandler::as<Function>(scriptobject)->call(ret,wrk,thisObj,args,numargs);
					}
					else if (asAtomHandler::is<AVM1Function>(scriptobject))
					{
						auto func = asAtomHandler::as<AVM1Function>(scriptobject);
						func->call(&ret,&thisObj,args,numargs);
					}
					else if (asAtomHandler::is<Class_base>(scriptobject))
					{
						Class_base* cls = asAtomHandler::as<Class_base>(scriptobject);
						if (!asAtomHandler::getObjectNoCheck(thisObj)->isConstructed() && !fromInitAction)
						{
							cls->handleConstruction(thisObj,args,numargs,true);
						}
						LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod from class done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
					}
					else if (asAtomHandler::isObject(scriptobject))
					{
						multiname m(nullptr);
						m.name_type=multiname::NAME_STRING;
						m.isAttribute = false;
						m.name_s_id = BUILTIN_STRINGS::STRING_CONSTRUCTOR;
						asAtom constr = asAtomHandler::invalidAtom;
						asAtomHandler::getObjectNoCheck(scriptobject)->getVariableByMultiname(constr,m,GET_VARIABLE_OPTION::NONE,wrk);
						if (asAtomHandler::isValid(constr))
						{
							if (asAtomHandler::is<Function>(constr))
							{
								asAtomHandler::as<Function>(constr)->call(ret,wrk,thisObj,args,numargs);
								asAtomHandler::as<Function>(constr)->decRef();
							}
							else if (asAtomHandler::is<AVM1Function>(constr))
							{
								auto f = asAtomHandler::as<AVM1Function>(constr);
								f->call(&ret,&thisObj,args,numargs,caller);
								f->decRef();
							}
							else if (asAtomHandler::is<Class_base>(constr))
							{
								Class_base* cls = asAtomHandler::as<Class_base>(constr);
								if (!asAtomHandler::getObjectNoCheck(thisObj)->isConstructed() && !fromInitAction)
									cls->handleConstruction(thisObj,args,numargs,true);
								LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod constructor from class done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
							}
						}
						else
							LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod without name and srciptobject has no constructor:"<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
					}
					else
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod without name and srciptobject is not a function:"<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
					done=true;
				}
				if (!done)
				{
					if (asAtomHandler::is<DisplayObject>(scriptobject))
					{
						AVM1Function* f = asAtomHandler::as<DisplayObject>(scriptobject)->AVM1GetFunction(nameID);
						if (f)
						{
							f->call(&ret,&scriptobject,args,numargs);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod from displayobject done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
							done=true;
						}
					}
				}
				if (asAtomHandler::isNull(scriptobject) || asAtomHandler::isUndefined(scriptobject))
				{
					ret=asAtomHandler::undefinedAtom;
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod from undefined/null ignored "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
					done=true;
				}
				if (!done)
				{
					uint32_t nameID = asAtomHandler::toStringId(name,wrk);
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;
					asAtom func=asAtomHandler::invalidAtom;
					if (asAtomHandler::isValid(scriptobject))
					{
						ASObject* scrobj = asAtomHandler::toObject(scriptobject,wrk);
						scrobj->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::DONT_CHECK_CLASS,wrk);
						if (!asAtomHandler::isValid(func))
						{
							// search the prototype before searching the class
							ASObject* pr =scrobj->getprop_prototype();
							while (pr)
							{
								pr->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
								if (asAtomHandler::isValid(func))
									break;
								pr = pr->getprop_prototype();
							}
						}
						if (!asAtomHandler::isValid(func))
						{
							scrobj->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
							if (!asAtomHandler::isValid(func) && scrobj->is<Class_base>())
								scrobj->as<Class_base>()->getClassVariableByMultiname(func,m,wrk,asAtomHandler::invalidAtom,UINT16_MAX);
						}
						if (!asAtomHandler::isValid(func))
						{
							if (scrobj->is<IFunction>())
							{
								ASObject* pr =asAtomHandler::getObject(scrobj->as<IFunction>()->prototype);
								while (pr)
								{
									pr->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
									if (asAtomHandler::isValid(func))
										break;
									pr = pr->getprop_prototype();
								}
							}
						}
						if (asAtomHandler::is<Function>(func))
						{
							asAtomHandler::as<Function>(func)->call(ret,wrk,scriptobject.uintval == super.uintval ? thisObj : scriptobject,args,numargs);
							asAtomHandler::as<Function>(func)->decRef();
						}
						else if (asAtomHandler::is<AVM1Function>(func))
						{
							auto f = asAtomHandler::as<AVM1Function>(func);
							if (scriptobject.uintval == super.uintval)
								f->call(&ret,&thisObj,args,numargs,caller);
							else
								f->call(&ret,&scriptobject,args,numargs,caller);
							f->decRef();
						}
						else
						{
							LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod function not found "<<asAtomHandler::toDebugString(scriptobject)<<" "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func));
							ret = asAtomHandler::undefinedAtom;
						}
					}
					else
						LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod invalid scriptobject "<<asAtomHandler::toDebugString(scriptobject)<<" "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func));
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallMethod done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject)<<" result:"<<asAtomHandler::toDebugString(ret));
				if (context->exceptionthrown && !context->exceptionthrown->is<StackOverflowError>())
				{
					if (tryblockstart== actionlist.end())
					{
						context->exceptionthrown->decRef();
						context->exceptionthrown=nullptr;
					}
				}
				PushStack(stack,ret);
				for (uint32_t i = 0; i < numargs; i++)
					ASATOM_REMOVESTOREDMEMBER(args[i]);
				ASATOM_DECREF(name);
				ASATOM_DECREF(scriptobject);
				ASATOM_DECREF(na);
				break;
			}
			case 0x53: // ActionNewMethod
			{
				asAtom name = PopStack(stack);
				asAtom scriptobject = PopStack(stack);
				asAtom na = PopStack(stack);
				size_t numargs = std::min((size_t)asAtomHandler::toUInt(na), stack.size());
				asAtom* args = LS_STACKALLOC(asAtom, numargs);
				for (size_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewMethod "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
				uint32_t nameID = asAtomHandler::toStringId(name,wrk);
				asAtom ret=asAtomHandler::invalidAtom;
				if (nameID == BUILTIN_STRINGS::EMPTY)
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewMethod without name "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				asAtom func=asAtomHandler::invalidAtom;
				ASObject* scrobj = asAtomHandler::toObject(scriptobject,wrk);
				bool funcrefcounted=false;
				if (asAtomHandler::isUndefined(name)|| asAtomHandler::toStringId(name,wrk) == BUILTIN_STRINGS::EMPTY)
				{
					func = scriptobject;
				}
				else
				{
					funcrefcounted=true;
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;
					scrobj->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
					if (!asAtomHandler::is<IFunction>(func) && !asAtomHandler::is<Class_base>(func))
					{
						ASATOM_DECREF(func);
						func = asAtomHandler::invalidAtom;
						ASObject* pr =scrobj->getprop_prototype();
						if (pr)
							pr->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
					}
				}
				asAtom tmp=asAtomHandler::invalidAtom;
				asAtom proto = asAtomHandler::invalidAtom;
				if (asAtomHandler::is<IFunction>(func))
				{
					proto = asAtomHandler::as<IFunction>(func)->getprop_prototypeAtom();
					if (!asAtomHandler::isObject(proto))
						proto = asAtomHandler::as<IFunction>(func)->prototype;
					ASObject* pr = asAtomHandler::getObject(proto);
					if (pr)
					{
						multiname mconstr(nullptr);
						mconstr.name_type=multiname::NAME_STRING;
						mconstr.name_s_id=BUILTIN_STRINGS::STRING_CONSTRUCTOR;
						mconstr.isAttribute = false;
						asAtom constr = asAtomHandler::invalidAtom;
						pr->getVariableByMultiname(constr,mconstr,GET_VARIABLE_OPTION::NONE,wrk);
						if (asAtomHandler::is<Class_base>(constr))
							asAtomHandler::as<Class_base>(constr)->getInstance(wrk,ret,true,nullptr,0);
						else if (pr->getClass())
							pr->getClass()->getInstance(wrk,ret,true,nullptr,0);
						asAtomHandler::toObject(ret,wrk)->setprop_prototype(proto);
						asAtomHandler::toObject(ret,wrk)->setprop_prototype(proto,BUILTIN_STRINGS::STRING_PROTO);
						ASATOM_DECREF(constr);
					}
				}
				if (asAtomHandler::is<Function>(func))
				{
					asAtomHandler::as<Function>(func)->call(tmp,wrk,ret,args,numargs);
				}
				else if (asAtomHandler::is<AVM1Function>(func))
				{
					auto f = asAtomHandler::as<AVM1Function>(func);
					f->call(nullptr,&ret,args,numargs,caller);
				}
				else if (asAtomHandler::is<Class_base>(func))
				{
					asAtomHandler::as<Class_base>(func)->getInstance(wrk,ret,true,args,numargs);
				}
				else
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewMethod function not found "<<asAtomHandler::toDebugString(scriptobject)<<" "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func));

				if (asAtomHandler::isObject(ret))
					asAtomHandler::getObjectNoCheck(ret)->setVariableAtomByQName(BUILTIN_STRINGS::STRING_CONSTRUCTOR,nsNameAndKind(),func,DYNAMIC_TRAIT,true,false);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewMethod done "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject)<<" result:"<<asAtomHandler::toDebugString(ret));
				if (context->exceptionthrown && !context->exceptionthrown->is<StackOverflowError>())
				{
					ASATOM_DECREF(ret);
					ret = asAtomHandler::undefinedAtom;
					context->exceptionthrown->decRef();
					context->exceptionthrown=nullptr;
				}
				if (funcrefcounted)
					ASATOM_DECREF(func);
				PushStack(stack,ret);
				ASATOM_DECREF(name);
				ASATOM_DECREF(scriptobject);
				ASATOM_DECREF(na);
				break;
			}
			case 0x54: // ActionInstanceOf
			{
				asAtom constr = PopStack(stack);
				asAtom obj = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionInstanceOf "<<asAtomHandler::toDebugString(constr)<<" "<<asAtomHandler::toDebugString(obj));
				ASObject* value = asAtomHandler::toObject(obj,wrk);
				ASObject* type = asAtomHandler::toObject(constr,wrk);
				bool implementsinterface= implementsInterface(constr,value,wrk);
				PushStack(stack,implementsinterface ? asAtomHandler::trueAtom : asAtomHandler::falseAtom);
				value->decRef();
				type->decRef();
				break;
			}
			case 0x55: // ActionEnumerate2
			{
				asAtom obj = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionEnumerate2 "<<asAtomHandler::toDebugString(obj));
				if (asAtomHandler::isObject(obj) && !asAtomHandler::isNumeric(obj))
				{
					ACTIONRECORD::PushStack(stack,asAtomHandler::undefinedAtom);
					ASObject* o = asAtomHandler::toObject(obj,wrk);
					o->AVM1enumerate(stack);
				}
				else
					PushStack(stack,asAtomHandler::undefinedAtom);
				ASATOM_DECREF(obj);
				break;
			}
			case 0x60: // ActionBitAnd
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				asAtom tmp = arg1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitAnd "<<asAtomHandler::toDebugString(arg1)<<" & "<<asAtomHandler::toDebugString(arg2));
				asAtomHandler::bit_and(arg1,wrk,arg2);
				PushStack(stack,arg1);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x61: // ActionBitOr
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				asAtom tmp = arg1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitOr "<<asAtomHandler::toDebugString(arg1)<<" | "<<asAtomHandler::toDebugString(arg2));
				asAtomHandler::bit_or(arg1,wrk,arg2);
				PushStack(stack,arg1);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x62: // ActionBitXOr
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				asAtom tmp = arg1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitXOr "<<asAtomHandler::toDebugString(arg1)<<" ^ "<<asAtomHandler::toDebugString(arg2));
				asAtomHandler::bit_xor(arg1,wrk,arg2);
				PushStack(stack,arg1);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x63: // ActionBitLShift
			{
				asAtom count = PopStack(stack);
				asAtom value = PopStack(stack);
				asAtom tmp = value;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitLShift "<<asAtomHandler::toDebugString(value)<<" << "<<asAtomHandler::toDebugString(count));
				asAtomHandler::lshift(value,wrk,count);
				PushStack(stack,value);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(count);
				break;
			}
			case 0x64: // ActionBitRShift
			{
				asAtom count = PopStack(stack);
				asAtom value = PopStack(stack);
				asAtom tmp = value;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitRShift "<<asAtomHandler::toDebugString(value)<<" >> "<<asAtomHandler::toDebugString(count));
				asAtomHandler::rshift(value,wrk,count);
				PushStack(stack,value);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(count);
				break;
			}
			case 0x65: // ActionBitURShift
			{
				auto swfVersion = clip->loadedFrom->version;
				asAtom count = PopStack(stack);
				asAtom value = PopStack(stack);
				asAtom tmp = value;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionBitURShift "<<asAtomHandler::toDebugString(value)<<" >> "<<asAtomHandler::toDebugString(count));
				asAtomHandler::urshift(value,wrk,count);
				// NOTE: In SWF 8-9, ActionBitURShift actually returns
				// a signed value.
				if (swfVersion == 8 || swfVersion == 9)
					asAtomHandler::setInt(value, wrk, asAtomHandler::toInt(value));
				PushStack(stack,value);
				ASATOM_DECREF(tmp);
				ASATOM_DECREF(count);
				break;
			}
			case 0x66: // ActionStrictEquals
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStrictEquals "<<asAtomHandler::toDebugString(arg2)<<" === "<<asAtomHandler::toDebugString(arg1));
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::AVM1isEqualStrict(arg2, arg1, wrk)));
				ASATOM_DECREF(arg1);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x67: // ActionGreater
			{
				asAtom arg1 = PopStack(stack);
				asAtom arg2 = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGreater "<<asAtomHandler::toDebugString(arg2)<<" > "<<asAtomHandler::toDebugString(arg1));
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::AVM1isLess(arg1,wrk,arg2) == TTRUE));
				ASATOM_DECREF(arg1);
				ASATOM_DECREF(arg2);
				break;
			}
			case 0x81: // ActionGotoFrame
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionGotoFrame "<<clip->toDebugString());
					break;
				}
				uint32_t frame = uint32_t(*it++) | ((*it++)<<8);
				if (invalidTarget)
				{
					break;
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGotoFrame "<<frame);
				clip->as<MovieClip>()->AVM1gotoFrame(frame,true,!clip->as<MovieClip>()->state.stop_FP,true);
				break;
			}
			case 0x83: // ActionGetURL
			{
				tiny_string s1((const char*)&(*it));
				it += s1.numBytes()+1;
				tiny_string s2((const char*)&(*it));
				it += s2.numBytes()+1;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetURL "<<s1<<" "<<s2);
				if (s2.startsWith("_level"))
				{
					if (s1 != "")
					{
						asAtom obj = asAtomHandler::fromObject(clip);
						int level =0;
						s2.substr(6,UINT32_MAX).toNumber(level);
						asAtom args[2];
						args[0] = asAtomHandler::fromString(wrk->getSystemState(),s1);
						args[1] = asAtomHandler::fromInt(level);
						asAtom ret = asAtomHandler::invalidAtom;
						MovieClip::AVM1LoadMovieNum(ret,wrk,obj,args,2);
					}
					else
					{
						asAtom obj = context->getVariable(thisObj, originalclip, clip, s2);
						asAtom ret = asAtomHandler::invalidAtom;
						if (asAtomHandler::is<MovieClip>(obj))
						{
							MovieClip::AVM1UnloadMovie(ret,wrk,obj,nullptr,0);
							ASATOM_DECREF(obj);
						}
						else
							LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetURL clip not found "<<s1<<" "<<s2<<" "<<asAtomHandler::toDebugString(obj));
					}
				}
				else
					clip->getSystemState()->openPageInBrowser(s1,s2);
				break;
			}
			case 0x87: // ActionStoreRegister
			{
				asAtom a = PeekStack(stack);
				ASATOM_ADDSTOREDMEMBER(a);
				uint8_t num = *it++;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStoreRegister "<<(int)num<<" "<<asAtomHandler::toDebugString(a));
				ASATOM_REMOVESTOREDMEMBER(registers[num]);
				registers[num] = a;
				break;
			}
			case 0x88: // ActionConstantPool
			{
				uint32_t c = uint32_t(*it++) | ((*it++)<<8);
				context->AVM1ClearConstants();
				for (uint32_t i = 0; i < c; i++)
				{
					tiny_string s((const char*)&(*it),true);
					it += s.numBytes()+1;
					context->AVM1AddConstant(sys->getUniqueStringId(s, true));
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionConstantPool "<<c);
				break;
			}
			case 0x8a: // ActionWaitForFrame
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionWaitForFrame "<<clip->toDebugString());
					break;
				}
				uint32_t frame = uint32_t(*it++) | ((*it++)<<8);
				uint32_t skipcount = (uint32_t)(*it++);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionWaitForFrame "<<frame<<"/"<<clip->as<MovieClip>()->getFrameContainer()->getFramesLoaded()<<" skip "<<skipcount);
				if (clip->as<MovieClip>()->getFrameContainer()->getFramesLoaded() <= frame && !clip->as<MovieClip>()->hasFinishedLoading())
				{
					// frame not yet loaded, skip actions
					while (skipcount && it != actionlist.end())
					{
						if (*it++ > 0x80)
						{
							uint32_t c = uint32_t(*it++) | ((*it++)<<8);
							it+=c;
						}
						skipcount--;
					}
				}
				break;
			}
			case 0x08: // ActionToggleQuality
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF3 DoActionTag ActionToggleQuality "<<hex<<(int)*it);
				break;
			case 0x8b: // ActionSetTarget
			{
				tiny_string s((const char*)&(*it));
				it += s.numBytes()+1;
				if (!clip)
				{
					LOG_CALL("AVM1: ActionSetTarget: setting target from undefined value to "<<s);
				}
				else
				{
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetTarget "<<s);
				}
				if (clip_isTarget)
					clip->decRef();
				clip_isTarget=false;
				if (s.empty())
				{
					clip = originalclip;
					invalidTarget=!clip->isOnStage();
				}
				else
				{
					auto newTarget = clip->AVM1GetClipFromPath(s);
					if (!clip->isOnStage())
					{
						// clip is not on stage, so we set invalidTarget
						// according to ruffle everything except playhead commands (play/stop/gotoframe...) is handled on root
						// see ruffle test avm1/removed_base_clip_tell_target
						invalidTarget=true;
						if (clip->getSystemState()->use_testrunner_date)
						{
							// Adobe doesn't display any message, but for some reason ruffle does,
							// so we do the same if we are in the testrunner
							tiny_string msg("Target not found: Target=\"");
							msg += s;
							msg += "\" Base=\"?\"";
							clip->getSystemState()->trace(msg);
						}
						newTarget=nullptr;
					}
					clip = newTarget != nullptr ? newTarget : clip->AVM1getRoot();
				}
				context->getScope()->setTargetScope(clip);
				break;
			}
			case 0x8c: // ActionGotoLabel
			{
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionGotoLabel "<<clip->toDebugString());
					break;
				}
				tiny_string s((const char*)&(*it));
				it += s.numBytes()+1;
				if (invalidTarget)
				{
					break;
				}
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGotoLabel "<<s);
				clip->as<MovieClip>()->AVM1gotoFrameLabel(s,true,true);
				break;
			}
			case 0x8e: // ActionDefineFunction2
			{
				tiny_string name((const char*)&(*it),true);
				it += name.numBytes()+1;
				uint32_t paramcount = uint32_t(*it++) | ((*it++)<<8);
				++it; //register count not used
				uint8_t flags = *it++;
				bool flag1 = flags&0x80;//PreloadParent
				bool flag2 = flags&0x40;//PreloadRoot
				bool flag3 = flags&0x20;//SuppressSuper
				bool flag4 = flags&0x10;//PreloadSuper
				bool flag5 = flags&0x08;//SuppressArguments
				bool flag6 = flags&0x04;//PreloadArguments
				bool flag7 = flags&0x02;//SuppressThis
				bool flag8 = flags&0x01;//PreloadThis
				bool flag9 = (*it++)&0x01;//PreloadGlobal
				std::vector<uint32_t> funcparamnames;
				std::vector<uint8_t> registernumber;
				for (uint16_t i=0; i < paramcount; i++)
				{
					uint8_t regnum = *it++;
					tiny_string n((const char*)&(*it),true);
					it += n.numBytes()+1;
					funcparamnames.push_back(sys->getUniqueStringId
					(
						n,
						context->isCaseSensitive()
					));
					registernumber.push_back(regnum);
				}
				uint32_t codesize = uint32_t(*it++) | ((*it++)<<8);
				vector<uint8_t> code;
				code.assign(it,it+codesize);
				it += codesize;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineFunction2 "<<name<<" "<<paramcount<<" "<<flag1<<flag2<<flag3<<flag4<<flag5<<flag6<<flag7<<flag8<<flag9<<" "<<codesize);
				AVM1Function* f = Class<IFunction>::getAVM1Function(wrk,clip,context,funcparamnames,code,context->getScope(),registernumber,flag1, flag2, flag3, flag4, flag5, flag6, flag7, flag8, flag9);
				//Create the prototype object
				ASObject* pr =new_asobject(f->getSystemState()->worker);
				pr->addStoredMember();
				f->prototype = asAtomHandler::fromObject(pr);
				f->incRef();
				pr->setVariableByQName("constructor","",f,DYNAMIC_TRAIT,false);
				f->setprop_prototype(f->prototype);
				f->setprop_prototype(f->prototype,BUILTIN_STRINGS::STRING_PROTO);
				if (name == "")
				{
					asAtom a = asAtomHandler::fromObject(f);
					PushStack(stack,a);
				}
				else
					clip->AVM1SetFunction(name,_MR(f));
				break;
			}
			case 0x94: // ActionWith
			{
				asAtom obj = PopStack(stack);
				uint32_t codesize = uint32_t(*it++) | ((*it++)<<8);
				auto itend = it;
				itend+=codesize;
				if (curdepth >= maxdepth)
				{
					it = itend;
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionWith depth exceeds maxdepth");
					break;
				}
				if (asAtomHandler::isNull(obj) || asAtomHandler::isUndefined(obj))
				{
					if (clip->getSystemState()->use_testrunner_date)
					{
						// Adobe doesn't display any message, but for some reason ruffle does,
						// so we do the same if we are in the testrunner
						wrk->getSystemState()->trace("Error: A 'with' action failed because the specified object did not exist.\n");
					}
					it = itend;
					break;
				}
				Log::calls_indent++;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionWith "<<codesize<<" "<<asAtomHandler::toDebugString(obj));
				++curdepth;
				scopestackstop[curdepth] = itend;
				context->getScope()->incRef();
				ASATOM_ADDSTOREDMEMBER(context->getScope()->getLocals());

				context->setScope(new AVM1Scope
								(
									context->getScope(),
									obj
								));
				ASATOM_DECREF(obj);
				break;
			}
			case 0x96: // ActionPush
			{
				uint32_t len = ((*(it-1))<<8) | (*(it-2));
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush start:"<<len);
				while (len > 0)
				{
					uint8_t type = *it++;
					len--;
					switch (type)
					{
						case 0:
						{
							tiny_string val((const char*)&(*it),true);
							len -= val.numBytes()+1;
							it += val.numBytes()+1;
							asAtom a = asAtomHandler::fromStringID(sys->getUniqueStringId(val, true));
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 0 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 1:
						{
							FLOAT f;
							f.read((const uint8_t*)&(*it));
							it+=4;
							len-=4;
							asAtom a = asAtomHandler::fromNumber(wrk,f,false);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 1 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 2:
							PushStack(stack,asAtomHandler::nullAtom);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 2 "<<asAtomHandler::toDebugString(asAtomHandler::nullAtom));
							break;
						case 3:
							PushStack(stack,asAtomHandler::undefinedAtom);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 3 "<<asAtomHandler::toDebugString(asAtomHandler::undefinedAtom));
							break;
						case 4:
						{
							uint32_t reg = *it++;
							len--;
							asAtom a = registers[reg];
							ASATOM_INCREF(a);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 4 register "<<reg<<" "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 5:
						{
							asAtom a = asAtomHandler::fromBool((bool)*it++);
							len--;
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 5 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 6:
						{
							DOUBLE d;
							d.read((const uint8_t*)&(*it));
							it+=8;
							len-=8;
							asAtom a = asAtomHandler::fromNumber(wrk,d,false);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 6 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 7:
						{
							uint32_t d=LS_UINT32_TO_LE(*(uint32_t*)&(*it));
							it+=4;
							len-=4;
							asAtom a = asAtomHandler::fromInt((int32_t)d);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 7 "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 8:
						{
							uint32_t index = (*it++);
							len--;
							asAtom a = context->AVM1GetConstant(index);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 8 "<<index<<" "<<asAtomHandler::toDebugString(a));
							break;
						}
						case 9:
						{
							uint32_t index = uint32_t(*it++) | ((*it++)<<8);
							len-=2;
							asAtom a = context->AVM1GetConstant(index);
							PushStack(stack,a);
							LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPush 9 "<<index<<" "<<asAtomHandler::toDebugString(a));
							break;
						}
						default:
							LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF4 DoActionTag push type "<<(int)type);
							break;
					}
				}
				break;
			}
			case 0x99: // ActionJump
			{
				int32_t skip = int16_t((*it++) | ((*it++)<<8));
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionJump "<<skip<<" "<< (it-actionlist.begin()));
				if (skip < 0 && it-actionlist.begin() < -skip)
				{
					LOG(LOG_ERROR,"ActionJump: invalid skip target:"<< skip<<" "<<(it-actionlist.begin())<<" "<<actionlist.size());
					it = actionlist.begin();
				}
				if (skip >=0 && skip + (it-actionlist.begin()) > (int)actionlist.size())
				{
					LOG(LOG_ERROR,"ActionJump: invalid skip target:"<< skip<<" "<<(it-actionlist.begin())<<" "<<actionlist.size());
					it = actionlist.end();
				}
				else
					it+= skip;
				break;
			}
			case 0x9a: // ActionGetURL2
			{
				asAtom at=PopStack(stack);
				asAtom au=PopStack(stack);
				uint8_t b = *it++;
				uint8_t method = b&0xc0>>6;
				bool loadtarget = b&0x02;
				bool loadvars = b&0x01;

				if (loadtarget) //LoadTargetFlag
					LOG(LOG_NOT_IMPLEMENTED,"AVM1: ActionGetURL2 with LoadTargetFlag");
				if (loadvars) //LoadVariablesFlag
					LOG(LOG_NOT_IMPLEMENTED,"AVM1: ActionGetURL2 with LoadVariablesFlag");
				if (method) //SendVarsMethod
					LOG(LOG_NOT_IMPLEMENTED,"AVM1: ActionGetURL2 with SendVarsMethod");
				tiny_string url = asAtomHandler::toString(au,wrk);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetURL2 "<<url<<" "<<asAtomHandler::toDebugString(at));
				if (asAtomHandler::is<MovieClip>(at))
				{
					asAtom ret = asAtomHandler::invalidAtom;
					if (url != "")
						MovieClip::AVM1LoadMovie(ret,wrk,at,&au,1);
					else
					{
						// it seems that calling ActionGetURL2 with a clip as target and no url unloads the clip
						MovieClip::AVM1UnloadMovie(ret,wrk,at,nullptr,0);
					}
				}
				else
				{
					tiny_string target = asAtomHandler::toString(at,wrk);
					clip->getSystemState()->openPageInBrowser(url,target);
				}
				ASATOM_DECREF(at);
				ASATOM_DECREF(au);
				break;
			}
			case 0x9b: // ActionDefineFunction
			{
				tiny_string name((const char*)&(*it),true);
				it += name.numBytes()+1;
				uint32_t paramcount = uint32_t(*it++) | ((*it++)<<8);
				std::vector<uint32_t> paramnames;
				for (uint16_t i=0; i < paramcount; i++)
				{
					tiny_string n((const char*)&(*it),true);
					it += n.numBytes()+1;
					paramnames.push_back(sys->getUniqueStringId(n, context->isCaseSensitive()));
				}
				uint32_t codesize = uint32_t(*it++) | ((*it++)<<8);
				vector<uint8_t> code;
				code.assign(it,it+codesize);
				it += codesize;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineFunction "<<name<<" "<<paramcount);
				AVM1Function* f = Class<IFunction>::getAVM1Function(wrk,clip,context,paramnames,code,context->getScope());
				//Create the prototype object
				ASObject* pr = new_asobject(f->getSystemState()->worker);
				f->prototype = asAtomHandler::fromObject(pr);
				pr->addStoredMember();
				f->incRef();
				pr->setVariableByQName("constructor","",f,DYNAMIC_TRAIT,false);
				f->setprop_prototype(f->prototype);
				f->setprop_prototype(f->prototype,BUILTIN_STRINGS::STRING_PROTO);
				if (name == "")
				{
					asAtom a = asAtomHandler::fromObject(f);
					PushStack(stack,a);
				}
				else
					clip->AVM1SetFunction(name,_MR(f));
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineFunction done "<<name<<" "<<f->toDebugString());
				break;
			}
			case 0x9d: // ActionIf
			{
				int32_t skip = int16_t((*it++) | ((*it++)<<8));
				asAtom a = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionIf "<<asAtomHandler::toDebugString(a)<<" "<<skip);
				if (asAtomHandler::AVM1toBool(a,wrk,clip->loadedFrom->version))
				{
					if (skip < 0 && it-actionlist.begin() < -skip)
					{
						LOG(LOG_ERROR,"ActionIf: invalid skip target:"<< skip<<" "<<(it-actionlist.begin())<<" "<<actionlist.size());
						it = actionlist.begin();
					}
					if (skip >=0 && skip + (it-actionlist.begin()) > (int)actionlist.size())
					{
						LOG(LOG_ERROR,"ActionIf: invalid skip target:"<< skip<<" "<<(it-actionlist.begin())<<" "<<actionlist.size());
						it = actionlist.end();
					}
					else
						it += skip;
				}
				ASATOM_DECREF(a);
				break;
			}
			case 0x9e: // ActionCall
			{
				asAtom a = PopStack(stack);
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionCall "<<clip->toDebugString());
					break;
				}
				if (asAtomHandler::isNumeric(a))
				{
					uint32_t frame = asAtomHandler::toUInt(a);
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCall frame "<<frame);
					clip->as<MovieClip>()->getFrameContainer()->AVM1ExecuteFrameActions(frame,clip->as<MovieClip>());
				}
				else
				{
					tiny_string s = asAtomHandler::AVM1toString(a,wrk);
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCall label "<<s);
					clip->as<MovieClip>()->AVM1ExecuteFrameActionsFromLabel(s);
				}
				ASATOM_DECREF(a);
				break;
			}
			case 0x9f: // ActionGotoFrame2
			{
				bool biasflag = (*it)&0x02;
				bool playflag = (*it)&0x01;
				it++;
				uint32_t biasframe = 0;
				if (biasflag)
					biasframe = uint32_t(*it++) | ((*it++)<<8);

				asAtom a = PopStack(stack);
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionGotoFrame2 "<<clip->toDebugString());
					ASATOM_DECREF(a);
					break;
				}
				if (invalidTarget)
				{
					ASATOM_DECREF(a);
					break;
				}
				if (asAtomHandler::isString(a))
				{
					tiny_string s = asAtomHandler::toString(a,wrk);
					if (biasframe)
						LOG(LOG_NOT_IMPLEMENTED,"AVM1: GotFrame2 with bias and label:"<<asAtomHandler::toDebugString(a));
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGotoFrame2 label "<<s);
					clip->as<MovieClip>()->AVM1gotoFrameLabel(s,!playflag,clip->as<MovieClip>()->state.stop_FP == playflag);

				}
				else
				{
					uint32_t frame = asAtomHandler::toUInt(a)+biasframe;
					LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGotoFrame2 "<<frame);
					if (frame>0) // variable frame is 1-based
						frame--;
					clip->as<MovieClip>()->AVM1gotoFrame(frame,!playflag,clip->as<MovieClip>()->state.stop_FP == playflag);
				}
				ASATOM_DECREF(a);
				break;
			}
			case 0x8d: // ActionWaitForFrame2
			{
				uint32_t skipcount= (*it);
				it++;
				asAtom a = PopStack(stack);
				if (!clip->is<MovieClip>())
				{
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" no MovieClip for ActionWaitForFrame2 "<<clip->toDebugString());
					ASATOM_DECREF(a);
					break;
				}
				uint32_t frame=0;
				if (asAtomHandler::isString(a))
				{
					tiny_string s = asAtomHandler::toString(a,wrk);
					frame = clip->as<MovieClip>()->getFrameContainer()->getFrameIdByLabel(s,"");
				}
				else
				{
					frame = asAtomHandler::toUInt(a);
				}
				if (clip->as<MovieClip>()->getFrameContainer()->getFramesLoaded() <= frame && !clip->as<MovieClip>()->hasFinishedLoading())
				{
					// frame not yet loaded, skip actions
					while (skipcount && it != actionlist.end())
					{
						it++;
						if (*it > 0x80)
						{
							uint32_t c = uint32_t(*it++) | ((*it++)<<8);
							it+=c;
						}
						skipcount--;
					}
				}
				ASATOM_DECREF(a);
				break;
			}
			case 0x8f: // ActionTry
			{
				bool catchInRegister = (*it)&0x04;
				it++;
				tryCatchBlock trycatchblock;
				trycatchblock.trysize = uint16_t(*it++) | ((*it++)<<8);
				trycatchblock.catchsize = uint16_t(*it++) | ((*it++)<<8);
				trycatchblock.finallysize = uint16_t(*it++) | ((*it++)<<8);
				trycatchblock.reg=UINT8_MAX;
				if (catchInRegister)
					trycatchblock.reg = *it++;
				else
				{
					trycatchblock.name = tiny_string((const char*)&(*it),true);
					it += trycatchblock.name.numBytes()+1;
				}
				trycatchblock.startpos=it-actionlist.begin();
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionTry "<<trycatchblock.name<<" "<<(int)trycatchblock.reg<<" "<<trycatchblock.trysize<<"/"<<trycatchblock.catchsize<<"/"<<trycatchblock.finallysize);
				if (trycatchblock.finallysize)
					LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionTry with finallysize:"<<trycatchblock.name<<" "<<(int)trycatchblock.reg<<" "<<trycatchblock.trysize<<"/"<<trycatchblock.catchsize<<"/"<<trycatchblock.finallysize);
				trycatchblocks.push_back(trycatchblock);
				tryblockstart = it;
				break;
			}
			case 0x33: // ActionAsciiToChar
			{
				asAtom a = PopStack(stack);
				uint16_t c = asAtomHandler::toUInt(a);
				auto id = sys->getUniqueStringId(tiny_string::fromChar(c), true);
				asAtom ret = asAtomHandler::fromStringID(id);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionAsciiToChar "<<c<<" "<<asAtomHandler::toDebugString(ret));
				ASATOM_DECREF(a);
				PushStack(stack,ret);
				break;
			}
			case 0x14: // ActionStringLength
			{
				asAtom a = PopStack(stack);
				tiny_string s= asAtomHandler::toString(a,wrk);
				asAtom ret = asAtomHandler::fromInt(s.numChars());
				ASATOM_DECREF(a);
				PushStack(stack,ret);
				break;
			}
			case 0x29: // ActionStringLess
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				tiny_string a = asAtomHandler::AVM1toString(aa,wrk);
				tiny_string b = asAtomHandler::AVM1toString(ab,wrk);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStringLess "<<a<<" "<<b<<" "<<(b<a));
				if (wrk->AVM1getSwfVersion() < 5)
					PushStack(stack,asAtomHandler::fromInt(b < a ? 1 : 0));
				else
					PushStack(stack,asAtomHandler::fromBool(b < a));
				break;
			}
			case 0x31: // ActionMBStringLength
			{
				LOG(LOG_NOT_IMPLEMENTED,"ActionMBStringLength doesn't check for multi-byte strings");
				asAtom a = PopStack(stack);
				tiny_string s= asAtomHandler::toString(a,wrk);
				asAtom ret = asAtomHandler::fromInt(s.numChars());
				ASATOM_DECREF(a);
				PushStack(stack,ret);
				break;
			}
			case 0x32: // ActionCharToAscii
			{
				asAtom a = PopStack(stack);
				tiny_string s= asAtomHandler::AVM1toString(a,wrk);
				asAtom ret = asAtomHandler::fromInt(s.empty() ? -1 : s.charAt(0));
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCharToAscii "<<s<<" "<<asAtomHandler::toDebugString(ret));
				ASATOM_DECREF(a);
				PushStack(stack,ret);
				break;
			}
			case 0x35: // ActionMBStringExtract
			{
				LOG(LOG_NOT_IMPLEMENTED,"ActionMBStringExtract doesn't check for multi-byte strings");
				asAtom count = PopStack(stack);
				asAtom index = PopStack(stack);
				asAtom str = PopStack(stack);
				tiny_string a = asAtomHandler::AVM1toString(str,wrk);
				// start parameter seems to be 1-based
				int startindex = asAtomHandler::toInt(index)-1;
				tiny_string b;
				if (uint32_t(startindex) <= a.numChars())
					b = a.substr(startindex,asAtomHandler::toInt(count));
				ASATOM_DECREF(count);
				ASATOM_DECREF(index);
				ASATOM_DECREF(str);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStringExtract "<<a<<" "<<b);
				PushStack(stack,asAtomHandler::fromString(clip->getSystemState(),b));
				break;
			}
			case 0x36: // ActionMBCharToAscii
			{
				LOG(LOG_NOT_IMPLEMENTED,"ActionMBCharToAscii doesn't check for multi-byte strings");
				asAtom a = PopStack(stack);
				tiny_string s= asAtomHandler::AVM1toString(a,wrk);
				asAtom ret = asAtomHandler::fromInt(s.empty() ? -1 : s.charAt(0));
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionMBCharToAscii "<<s<<" "<<asAtomHandler::toDebugString(ret));
				ASATOM_DECREF(a);
				PushStack(stack,ret);
				break;
			}
			case 0x37: // ActionMBAsciiToChar
			{
				LOG(LOG_NOT_IMPLEMENTED,"ActionMBAsciiToChar doesn't check for multi-byte strings");
				asAtom a = PopStack(stack);
				uint16_t c = asAtomHandler::toUInt(a);
				auto id = sys->getUniqueStringId(tiny_string::fromChar(c), true);
				asAtom ret = asAtomHandler::fromStringID(id);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionMBAsciiToChar "<<c<<" "<<asAtomHandler::toDebugString(ret));
				ASATOM_DECREF(a);
				PushStack(stack,ret);
				break;
			}
			case 0x45: // ActionTargetPath
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF5 DoActionTag "<<hex<<(int)opcode);
				break;
			case 0x68: // ActionStringGreater
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				tiny_string a = asAtomHandler::AVM1toString(aa,wrk);
				tiny_string b = asAtomHandler::AVM1toString(ab,wrk);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStringGreater "<<a<<" "<<b<<" "<<(b<a));
				if (wrk->AVM1getSwfVersion() < 5)
					PushStack(stack,asAtomHandler::fromInt(b > a ? 1 : 0));
				else
					PushStack(stack,asAtomHandler::fromBool(b > a));
				break;
			}
			case 0x2c: // ActionImplementsOp
			{
				asAtom cls = PopStack(stack);
				asAtom interfacecount = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionImplementsOp "<<asAtomHandler::toDebugString(cls)<<" "<<asAtomHandler::toDebugString(interfacecount));
				uint32_t c= asAtomHandler::toUInt(interfacecount);
				if (!asAtomHandler::is<AVM1Function>(cls))
					LOG(LOG_ERROR,"ActionImplementsOp with invalit constructor argument:"<<asAtomHandler::toDebugString(cls));
				else
				{
					AVM1Function* f = asAtomHandler::as<AVM1Function>(cls);
					for (uint32_t i = 0; i < c; i++)
					{
						asAtom iface = PopStack(stack);
						f->addInterface(iface);
					}
				}
				ASATOM_DECREF(cls);
				break;
			}
			default:
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" invalid DoActionTag "<<hex<<(int)opcode);
				throw RunTimeException("invalid AVM1 tag");
				break;
		}
		++context->actionsExecuted;
	}
	while (curdepth > 0)
	{
		// ActionReturn reached inside "with"
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" end with "<<asAtomHandler::toDebugString(context->getScope()->getLocals()));
		AVM1Scope* sc = context->getScope()->getParentScope();
		if (sc)
		{
			sc->incRef();
			ASATOM_REMOVESTOREDMEMBER(sc->getLocals());
		}
		context->setScope(sc);
		curdepth--;
		Log::calls_indent--;
	}
	if (scope && isClosure)
	{
		scope->incRef();
		context->setScope(scope);
	}
	else
	{
		context->setScope(nullptr);
		context->setGlobalScope(nullptr);
	}

	for (uint32_t i = 0; i < 256; i++)
	{
		ASATOM_REMOVESTOREDMEMBER(registers[i]);
	}
	while (!stack.empty())
	{
		asAtom a = PopStack(stack);
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" stack not empty:"<<asAtomHandler::toDebugString(a));
		ASATOM_DECREF(a);
	}
	if (clip_isTarget)
		clip->decRef();
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" executeActions done");
	Log::calls_indent--;
	context->callDepth--;
	wrk->AVM1_cur_recursion_function--;
	if (isInternalCall && !context->exceptionthrown)
		wrk->AVM1_cur_recursion_internal--;
	wrk->AVM1callStack.pop_back();
	if (wrk->AVM1_cur_recursion_internal>65 || (context->exceptionthrown && context->callDepth==0 ))
	{
		if (wrk->AVM1callStack.empty())
		{
			LOG(LOG_ERROR,"AVM1: unhandled exception:"<<context->exceptionthrown->toDebugString());
			context->exceptionthrown->decRef();
			context->exceptionthrown=nullptr;
		}
		else if (wrk->AVM1callStack.back() != context)
		{
			wrk->AVM1callStack.back()->exceptionthrown=context->exceptionthrown;
			context->exceptionthrown=nullptr;
		}
	}
}
