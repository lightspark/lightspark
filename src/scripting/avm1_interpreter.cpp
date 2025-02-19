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

// Based on Ruffle's `avm1::activation::Activation::resolve_target_path()`.
ASObject* AVM1context::resolveTargetPath
(
	const asAtom& thisObj,
	DisplayObject* baseClip,
	DisplayObject* root,
	const _R<ASObject>& start,
	const tiny_string& _path,
	bool hasSlash,
	bool first
) const
{
	auto path = _path;
	// An empty path resolves to the starting clip.
	if (path.empty())
	{
		start->incRef();
		return start.getPtr();
	}

	ASObject* obj = start.getPtr();
	bool isSlashPath;
	// A starting `/` means we have an absolute path, starting from the
	// root. e.g. `/foo` == `_root.foo`.
	if ((isSlashPath = path.startsWith("/")))
	{
		path = path.substr(1, UINT32_MAX);
		obj = root;
	}

	while (!path.empty())
	{
		// Skip over any leading `:`s.
		// e.g. `:foo`, and `::foo` are the same as `foo`.
		path = path.trimStartMatches(':');

		asAtom val = asAtomHandler::invalidAtom;
		auto clip = obj->is<DisplayObject>() ? obj->as<DisplayObject>() : nullptr;

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
				val = thisObj;
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
					obj->AVM1getVariableByMultiname
					(
						val,
						objName,
						GET_VARIABLE_OPTION::NO_INCREF,
						// TODO: Uncomment this, once this is `virtual`,
						// and an overload for `DisplayObject`s is added.
						//hasSlash,
						baseClip->getInstanceWorker()
					);
				}
			}
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

	obj->incRef();
	return obj;
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
		scope->forEachScope([&](const AVM1Scope& scope)
		{
			auto obj = resolveTargetPath
			(
				thisObj,
				baseClip,
				root,
				scope.getLocals(),
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
		scope->forEachScope([&](const AVM1Scope& scope)
		{
			auto obj = resolveTargetPath
			(
				thisObj,
				baseClip,
				root,
				scope.getLocals(),
				path,
				true,
				pathHasSlash
			);

			if (obj == nullptr)
				return true;

			obj->incRef();
			ret = asAtomHandler::fromObject(obj);
			return false;
		});

		if (asAtomHandler::isValid(ret))
			return ret;
	}

	if (path == "this")
	{
		ASATOM_INCREF(thisObj);
		return thisObj;
	}
	else if (path == "_global")
	{
		sys->avm1global->incRef();
		return asAtomHandler::fromObject(sys->avm1global);
	}

	// Check for level names (`_level<depth>`).
	auto prefix = path.substr(0, 6);
	bool isLevelName =
	(
		prefix.equalsWithCase("_level", isCaseSensitive()) ||
		prefix.equalsWithCase("_flash", isCaseSensitive())
	);
	if (isLevelName)
	{
		if (path.numBytes() == 7 && path[6] == '0')
		{
			auto root = baseClip->AVM1getRoot();
			root->incRef();
			return asAtomHandler::fromObject(root);
		}

		// TODO: Support level names other than `_level0`.
		LOG
		(
			LOG_NOT_IMPLEMENTED,
			"getVariable: level names other than `" << prefix <<
			"0`. Got " << path
		);
		return asAtomHandler::undefinedAtom;
	}

	multiname m(nullptr);
	m.name_type = multiname::NAME_STRING;
	m.isAttribute = false;
	m.name_s_id = sys->getUniqueStringId
	(
		path,
		// NOTE: Internal property keys are always case insensitive
		// (e.g. `_x`, `_y`, etc).
		isCaseSensitive() && !path.startsWith("_")
	);

	// It's a normal variable name.
	// Resolve it using the scope chain.
	auto atom = scope->getVariableByMultiname
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
		return asAtomHandler::undefinedAtom;

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
		ASATOM_INCREF(value);
		thisObj = value;
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

		scope->forEachScope([&](const AVM1Scope& scope)
		{
			auto obj = resolveTargetPath
			(
				thisObj,
				baseClip,
				root,
				scope.getLocals(),
				varPath,
				true,
				pathHasSlash
			);

			if (obj == nullptr || !obj->hasPropertyByMultiname(m, true, true, wrk))
				return true;

			bool alreadySet = obj->AVM1setVariableByMultiname
			(
				m,
				(asAtom&)value,
				ASObject::CONST_ALLOWED,
				wrk
			);
			if (alreadySet)
				ASATOM_DECREF(value)
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
		ASObject::CONST_ALLOWED,
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
Mutex executeactionmutex;
void ACTIONRECORD::executeActions(DisplayObject *clip, AVM1context* context, const std::vector<uint8_t> &actionlist, uint32_t startactionpos, const _NR<AVM1Scope>& scope, bool fromInitAction, asAtom* result, asAtom* obj, asAtom *args, uint32_t num_args, const std::vector<uint32_t>& paramnames, const std::vector<uint8_t>& paramregisternumbers,
								  bool preloadParent, bool preloadRoot, bool suppressSuper, bool preloadSuper, bool suppressArguments, bool preloadArguments, bool suppressThis, bool preloadThis, bool preloadGlobal, AVM1Function *caller, AVM1Function *callee, Activation_object *actobj, asAtom *superobj)
{
	Locker l(executeactionmutex);
	bool clip_isTarget=false;
	assert(!clip->needsActionScript3());
	SystemState* sys = clip->getSystemState();
	ASWorker* wrk = sys->worker;
	if (context->callDepth == 0)
		context->startTime = compat_now();
	if (context->callDepth >= wrk->limits.max_recursion - 1)
	{
		std::stringstream s;
		s << "Reached maximum function recursion limit of " << wrk->limits.max_recursion;
		throw ScriptLimitException
		(
			s.str(),
			ScriptLimitException::MaxFunctionRecursion
		);
	}
	context->swfversion=clip->loadedFrom->version;
	context->callee = callee;

	if (context->globalScope.isNull())
	{
		auto global = clip->getSystemState()->avm1global;
		context->globalScope = _MNR(new AVM1Scope(_MR(global)));
	}

	bool isClosure = context->swfversion >= 6;

	if (!isClosure || scope.isNull())
		clip->incRef();
	// NOTE: In SWF 5, function calls aren't closures, and always create
	// a new target scope, regardless of the function's origin.
	auto parentScope = isClosure && !scope.isNull() ? scope : _MNR(new AVM1Scope
	(
		context->globalScope,
		_MR(clip)
	));

	// Local scopes are only created for function calls.
	context->scope = scope.isNull() ? parentScope : _MNR
	(
		new AVM1Scope(parentScope, wrk)
	);

	wrk->AVM1callStack.push_back(context);
	context->callDepth++;
	Log::calls_indent++;
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" executeActions "<<preloadParent<<preloadRoot<<suppressSuper<<preloadSuper<<suppressArguments<<preloadArguments<<suppressThis<<preloadThis<<preloadGlobal<<" "<<startactionpos<<" "<<num_args);
	if (result)
		asAtomHandler::setUndefined(*result);
	std::stack<asAtom> stack;
	asAtom registers[256];
	std::fill_n(registers,256,asAtomHandler::undefinedAtom);
	int curdepth = 0;
	int maxdepth = context->swfversion < 6 ? 8 : 16;
	auto thisObj = obj != nullptr ? *obj : asAtomHandler::fromObject(clip);
	std::vector<uint8_t>::const_iterator* scopestackstop = g_newa(std::vector<uint8_t>::const_iterator, maxdepth);
	scopestackstop[0] = actionlist.end();
	uint32_t currRegister = 1; // spec is not clear, but gnash starts at register 1
	if (!suppressThis || preloadThis)
	{
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" preload this:"<<asAtomHandler::toDebugString(thisObj));
		if (!suppressThis)
			ASATOM_INCREF(thisObj);
		registers[currRegister++] = !suppressThis ? thisObj : asAtomHandler::undefinedAtom;
	}

	AVM1Array* argsArray = nullptr;
	if (!suppressArguments || preloadArguments)
	{
		argsArray = Class<AVM1Array>::getInstanceS(wrk);
		argsArray->resize(num_args);
		for (uint32_t i = 0; i < num_args; i++)
		{
			ASATOM_INCREF(args[i]);
			LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" parameter "<<i<<" "<<(paramnames.size() >= num_args ? clip->getSystemState()->getStringFromUniqueId(paramnames[i]):"")<<" "<<asAtomHandler::toDebugString(args[i]));
			argsArray->set(i,args[i],false,false);
			argsArray->addEnumerationValue(i,true);
		}

		auto proto = _MNR(argsArray->getClass()->prototype->getObj());
		argsArray->setprop_prototype(proto, BUILTIN_STRINGS::STRING_PROTO);

		nsNameAndKind tmpns(clip->getSystemState(), "", NAMESPACE);
		asAtom c = caller ? asAtomHandler::fromObject(caller) : asAtomHandler::nullAtom;

		if (caller)
			caller->incRef();

		argsArray->setVariableAtomByQName("caller", tmpns, c, DYNAMIC_TRAIT, false, 5);

		if (callee)
		{
			callee->incRef();
			asAtom c = asAtomHandler::fromObject(callee);
			argsArray->setVariableAtomByQName("callee", tmpns, c, DYNAMIC_TRAIT, false, 5);
		}

		argsArray->incRef();
		auto argsAtom = asAtomHandler::fromObject(argsArray);
		if (preloadArguments)
			registers[currRegister++] = argsAtom;
		else
		{
			context->scope->forceDefineLocal
			(
				"arguments",
				argsAtom,
				ASObject::CONST_ALLOWED,
				wrk
			);
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
		ASATOM_INCREF(super);
		if (preloadSuper)
		{
			// NOTE: The `super` register is set to `undefined`, if both
			// flags are set.
			registers[currRegister++] = (!suppressSuper) ? super : asAtomHandler::undefinedAtom;
		}
		else
		{
			context->scope->forceDefineLocal
			(
				BUILTIN_STRINGS::STRING_SUPER,
				super,
				ASObject::CONST_ALLOWED,
				wrk
			);
		}
	}
	if (preloadRoot)
	{
		DisplayObject* root=clip->AVM1getRoot();
		root->incRef();
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
			registers[currRegister++] = asAtomHandler::fromObject(clip->getParent());
		}
	}
	if (preloadGlobal)
	{
		clip->getSystemState()->avm1global->incRef();
		registers[currRegister++] = asAtomHandler::fromObject(clip->getSystemState()->avm1global);
	}
	for (uint32_t i = 0; i < paramnames.size() && i < num_args; i++)
	{
		auto reg = i < paramregisternumbers.size() ? paramregisternumbers[i] : 0;
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" set argument "<<i<<" "<<(int)reg<<" "<<clip->getSystemState()->getStringFromUniqueId(paramnames[i])<<" "<<asAtomHandler::toDebugString(args[i]));
		ASATOM_INCREF(args[i]);

		if (reg != 0)
		{
			ASATOM_INCREF(args[i]);
			ASATOM_DECREF(registers[reg]);
			registers[reg] = args[i];
		}
		else
		{
			context->scope->forceDefineLocal
			(
				paramnames[i],
				args[i],
				ASObject::CONST_ALLOWED,
				wrk
			);
		}
	}
	std::vector<tryCatchBlock> trycatchblocks;
	bool inCatchBlock = false;

	DisplayObject *originalclip = clip;
	auto it = actionlist.begin()+startactionpos;
	auto tryblockstart = actionlist.end();
	while (it != actionlist.end())
	{
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
					context->scope->setVariableByMultiname
					(
						m,
						e,
						ASObject::CONST_ALLOWED,
						wrk
					);
				}
				else if(trycatchblock.reg != UINT8_MAX)
				{
					ASATOM_DECREF(registers[trycatchblock.reg]);
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
			LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" end with "<<asAtomHandler::toDebugString(scopestack[curdepth]));
			// `incRef()` the parent before setting the scope, to prevent
			// a potential premature `free()`.
			context->scope->getParentPtr()->incRef();
			context->scope = context->scope->getParent();
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
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<< " "<<int(it-actionlist.begin())<< " action code:"<<hex<<(int)*it<<dec<<" "<<clip->toDebugString());
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
				// so we call in the same order that adobe does
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
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
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
				PushStack(stack,asAtomHandler::fromBool(b==a));
				break;
			}
			case 0x0f: // ActionLess
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionLess "<<b<<"<"<<a);
				PushStack(stack,asAtomHandler::fromBool(b<a));
				break;
			}
			case 0x10: // ActionAnd
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionAnd "<<b<<" && "<<a);
				PushStack(stack,asAtomHandler::fromBool(b && a));
				break;
			}
			case 0x11: // ActionOr
			{
				asAtom aa = PopStack(stack);
				asAtom ab = PopStack(stack);
				number_t a = asAtomHandler::AVM1toNumber(aa,clip->loadedFrom->version);
				number_t b = asAtomHandler::AVM1toNumber(ab,clip->loadedFrom->version);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionOr "<<b<<" || "<<a);
				PushStack(stack,asAtomHandler::fromBool(b || a));
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
				tiny_string a = asAtomHandler::toString(aa,wrk);
				tiny_string b = asAtomHandler::toString(ab,wrk);
				ASATOM_DECREF(aa);
				ASATOM_DECREF(ab);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStringEquals "<<a<<" "<<b);
				PushStack(stack,asAtomHandler::fromBool(a == b));
				break;
			}
			case 0x15: // ActionStringExtract
			{
				asAtom count = PopStack(stack);
				asAtom index = PopStack(stack);
				asAtom str = PopStack(stack);
				tiny_string a = asAtomHandler::toString(str,wrk);
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionPop");
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetVariable "<<asAtomHandler::toDebugString(name));
				auto s = asAtomHandler::AVM1toString(name, wrk);
				auto res = context->getVariable(thisObj, originalclip, clip, s);
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
					}
				}
				clip->incRef();
				context->scope->setTargetScope(_MR(clip));
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
				clip->getSystemState()->trace(asAtomHandler::toString(value,wrk));
				ASATOM_DECREF(value);
				break;
			}
			case 0x27: // ActionStartDrag
			{
				asAtom target = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStartDrag "<<asAtomHandler::toDebugString(target));
				asAtom lockcenter = PopStack(stack);
				asAtom constrain = PopStack(stack);
				Rectangle* rect = nullptr;
				if (asAtomHandler::toInt(constrain))
				{
					asAtom y2 = PopStack(stack);
					asAtom x2 = PopStack(stack);
					asAtom y1 = PopStack(stack);
					asAtom x1 = PopStack(stack);
					rect = Class<Rectangle>::getInstanceS(wrk);
					asAtom ret=asAtomHandler::invalidAtom;
					asAtom obj = asAtomHandler::fromObject(rect);
					Rectangle::_setTop(ret,wrk,obj,&y1,1);
					Rectangle::_setLeft(ret,wrk,obj,&x1,1);
					Rectangle::_setBottom(ret,wrk,obj,&y2,1);
					Rectangle::_setRight(ret,wrk,obj,&x2,1);
				}
				asAtom obj=asAtomHandler::invalidAtom;
				if (asAtomHandler::is<MovieClip>(target))
				{
					obj = target;
				}
				else
				{
					tiny_string s = asAtomHandler::toString(target,wrk);
					ASATOM_DECREF(target);
					DisplayObject* targetclip = clip->AVM1GetClipFromPath(s);
					if (targetclip)
					{
						obj = asAtomHandler::fromObject(targetclip);
					}
					else
					{
						LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStartDrag target clip not found:"<<asAtomHandler::toDebugString(target));
						break;
					}
				}
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom args[2];
				args[0] = lockcenter;
				if (rect)
					args[1] = asAtomHandler::fromObject(rect);
				Sprite::_startDrag(ret,wrk,obj,args,rect ? 2 : 1);
				ASATOM_DECREF(lockcenter);
				ASATOM_DECREF(constrain);
				break;
			}
			case 0x28: // ActionEndDrag
			{
				asAtom ret=asAtomHandler::invalidAtom;
				asAtom obj = asAtomHandler::fromObject(clip);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionEndDrag "<<asAtomHandler::toDebugString(obj));
				Sprite::_stopDrag(ret,wrk,obj,nullptr,0);
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
				if (type == Class<ASObject>::getRef(clip->getSystemState()).getPtr() || ABCVm::instanceOf(value,type))
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
				c->prototype = _MNR(new_asobject(wrk));
				c->prototype->addStoredMember();
				c->prototype->incRef();
				_NR<ASObject> probj = _MR(c->prototype);
				c->setprop_prototype(probj);
				c->setprop_prototype(probj,BUILTIN_STRINGS::STRING_PROTO);
				ASObject* superobj = asAtomHandler::toObject(superconstr,wrk);
				ASObject* proto = superobj->getprop_prototype();
				if (!proto)
				{
					if (superobj->is<Class_base>())
						proto = superobj->as<Class_base>()->getPrototype(wrk)->getObj();
					else if (superobj->is<IFunction>())
						proto = superobj->as<IFunction>()->prototype.getPtr();
				}
				if (proto)
				{
					proto->incRef();
					_NR<ASObject> o = _MR(proto);
					c->prototype->setprop_prototype(o);
					c->prototype->setprop_prototype(o,BUILTIN_STRINGS::STRING_PROTO);
				}
				else
					LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionExtends  super prototype not found "<<asAtomHandler::toDebugString(superconstr)<<" "<<asAtomHandler::toDebugString(subconstr));
				c->prototype->setVariableAtomByQName("constructor",nsNameAndKind(),superconstr, DECLARED_TRAIT);
				if (c->is<AVM1Function>())
					c->as<AVM1Function>()->setSuper(superconstr);
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

				bool ret = context->scope->deleteVariableByMultiname(m, wrk);

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

				context->scope->defineLocalByMultiname
				(
					m,
					value,
					ASObject::CONST_ALLOWED,
					wrk
				);

				ASATOM_DECREF(name);
				break;
			}
			case 0x3d: // ActionCallFunction
			{
				asAtom name = PopStack(stack);
				asAtom na = PopStack(stack);
				uint32_t numargs = asAtomHandler::toUInt(na);
				asAtom* args = numargs ? g_newa(asAtom, numargs) : nullptr;
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction "<<asAtomHandler::toDebugString(name)<<" "<<numargs);

				asAtom ret = asAtomHandler::undefinedAtom;
				auto s = asAtomHandler::AVM1toString(name, wrk);
				auto func = context->getVariable(thisObj, originalclip, clip, s);

				if (asAtomHandler::is<AVM1Function>(func))
				{
					asAtom obj = asAtomHandler::fromObjectNoPrimitive(clip);
					auto f = asAtomHandler::as<AVM1Function>(func);
					f->call(&ret,&obj,args,numargs,caller);
					f->decRef();
				}
				else if (asAtomHandler::is<Function>(func))
				{
					asAtom obj = asAtomHandler::fromObjectNoPrimitive(clip);
					asAtomHandler::as<Function>(func)->call(ret,wrk,obj,args,numargs);
					asAtomHandler::as<Function>(func)->decRef();
				}
				else if (asAtomHandler::is<Class_base>(func))
					asAtomHandler::as<Class_base>(func)->generator(wrk,ret,args,numargs);
				else
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction function not found "<<asAtomHandler::toDebugString(name)<<" "<<asAtomHandler::toDebugString(func)<<" "<<numargs);

				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionCallFunction done "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				for (uint32_t i = 0; i < numargs; i++)
					ASATOM_DECREF(args[i]);
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
				asAtom* args = g_newa(asAtom, numargs);
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

					auto cls = context->scope->getVariableByMultiname
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
						ASObject* o = new_functionObject(asAtomHandler::as<AVM1Function>(cls)->prototype);
						o->setprop_prototype(asAtomHandler::as<AVM1Function>(cls)->prototype);
						o->setprop_prototype(asAtomHandler::as<AVM1Function>(cls)->prototype,BUILTIN_STRINGS::STRING_PROTO);
						ret = asAtomHandler::fromObject(o);
						auto func = asAtomHandler::as<AVM1Function>(cls);
						func->call(nullptr,&ret, args,numargs,caller);
						func->decRef();
					}
				}
				if (asAtomHandler::isInvalid(ret))
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewObject class not found "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
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

				if (!context->scope->getLocals()->hasPropertyByMultiname(m, true, false, wrk))
				{
					auto atom = asAtomHandler::undefinedAtom;
					context->scope->defineLocalByMultiname(m, atom, ASObject::CONST_ALLOWED, wrk);
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
					ret->setDynamicVariableNoCheck(nameid,value,asAtomHandler::isInteger(name));
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
					res = asAtomHandler::fromString(clip->getSystemState(),"movieclip");
				else if (asAtomHandler::is<Date>(obj))
					res = asAtomHandler::fromStringID(BUILTIN_STRINGS::STRING_STRING);
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
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::isLess(arg2,wrk,arg1) == TTRUE));
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
						o->AVM1getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::DONT_CHECK_CLASS,wrk);
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
									ret = asAtomHandler::fromObject(o->as<IFunction>()->prototype.getPtr());
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
										if (o->is<Class_base>())
										{
											ret = asAtomHandler::fromObject(o->as<Class_base>()->getPrototype(wrk)->getObj());
											ASATOM_INCREF(ret);
										}
										else if (o->getClass())
										{
											ret = asAtomHandler::fromObject(o->getClass()->getPrototype(wrk)->getObj());
											ASATOM_INCREF(ret);
										}
										else
											LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember for scriptobject without class "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
										break;
									}
									case BUILTIN_STRINGS::STRING_AVM1_TARGET:
										if (o->is<DisplayObject>())
											ret = asAtomHandler::fromString(clip->getSystemState(),o->as<DisplayObject>()->AVM1GetPath());
										else
											asAtomHandler::setUndefined(ret);
										break;
									default:
									{
										o->AVM1getVariableByMultiname(ret,m,GET_VARIABLE_OPTION::NONE,wrk);
										break;
									}
								}
								break;
							default:
								LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGetMember for scriptobject type "<<asAtomHandler::toDebugString(scriptobject)<<" " <<asAtomHandler::toDebugString(name));
								break;
						}
					}
					if (asAtomHandler::isInvalid(ret))
					{
						if (o && o->is<DisplayObject>())
							ret = o->as<DisplayObject>()->AVM1GetVariable(asAtomHandler::toString(name,wrk),false);
					}
					if (context->exceptionthrown)
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
					if (o->is<DisplayObject>())
						ASATOM_INCREF(value);// incref here to make sure value is not destructed when setting value, reference is consumed in AVM1SetVariableDirect
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
						ASObject* protobj = asAtomHandler::toObject(value,wrk);
						if (protobj)
						{
							protobj->incRef();
							_NR<ASObject> p = _MR(protobj);
							o->setprop_prototype(p);
							o->setprop_prototype(p,BUILTIN_STRINGS::STRING_PROTO);
						}
						else
							LOG(LOG_ERROR,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionSetMember no prototype found for "<<asAtomHandler::toDebugString(scriptobject)<<" "<<asAtomHandler::toDebugString(value));
					}
					else
					{
						if (asAtomHandler::is<AVM1Function>(value))
						{
							AVM1Function* f = asAtomHandler::as<AVM1Function>(value);
							if (f->needsSuper())
							{
								asAtom a =asAtomHandler::as<AVM1Function>(value)->getSuper();
								if (asAtomHandler::isInvalid(a))
								{
									ASObject* sup = o->getprop_prototype();
									if (!sup && o->getClass() != Class<ASObject>::getRef(wrk->getSystemState()).getPtr())
										sup = o->getClass();
									if (sup)
										asAtomHandler::as<AVM1Function>(value)->setSuper(asAtomHandler::fromObjectNoPrimitive(sup));
								}
							}
						}
						(void)o->AVM1setVariableByMultiname(m,value,ASObject::CONST_ALLOWED,wrk);
					}
					if (o->is<DisplayObject>())
					{
						o->as<DisplayObject>()->AVM1UpdateVariableBindings(nameID, value);
						o->as<DisplayObject>()->AVM1SetVariableDirect(nameID, value);
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
				asAtom* args = numargs ? g_newa(asAtom, numargs) : nullptr;
				for (uint32_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
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
								scrobj->as<Class_base>()->getClassVariableByMultiname(func,m,wrk,asAtomHandler::invalidAtom);
						}
						if (!asAtomHandler::is<IFunction>(func))
						{
							if (scrobj->is<IFunction>())
							{
								ASObject* pr =scrobj->as<IFunction>()->prototype.getPtr();
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
				if (context->exceptionthrown)
				{
					if (tryblockstart== actionlist.end())
					{
						context->exceptionthrown->decRef();
						context->exceptionthrown=nullptr;
					}
				}
				PushStack(stack,ret);
				for (uint32_t i = 0; i < numargs; i++)
					ASATOM_DECREF(args[i]);
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
				asAtom* args = g_newa(asAtom, numargs);
				for (size_t i = 0; i < numargs; i++)
					args[i] = PopStack(stack);
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewMethod "<<asAtomHandler::toDebugString(name)<<" "<<numargs<<" "<<asAtomHandler::toDebugString(scriptobject));
				uint32_t nameID = asAtomHandler::toStringId(name,wrk);
				asAtom ret=asAtomHandler::invalidAtom;
				if (nameID == BUILTIN_STRINGS::EMPTY)
					LOG(LOG_NOT_IMPLEMENTED, "AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionNewMethod without name "<<asAtomHandler::toDebugString(name)<<" "<<numargs);
				asAtom func=asAtomHandler::invalidAtom;
				ASObject* scrobj = asAtomHandler::toObject(scriptobject,wrk);
				if (asAtomHandler::isUndefined(name)|| asAtomHandler::toStringId(name,wrk) == BUILTIN_STRINGS::EMPTY)
				{
					func = scriptobject;
				}
				else
				{
					multiname m(nullptr);
					m.name_type=multiname::NAME_STRING;
					m.name_s_id=nameID;
					m.isAttribute = false;
					scrobj->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
					if (!asAtomHandler::is<IFunction>(func))
					{
						ASObject* pr =scrobj->getprop_prototype();
						if (pr)
							pr->getVariableByMultiname(func,m,GET_VARIABLE_OPTION::NONE,wrk);
					}
				}
				asAtom tmp=asAtomHandler::invalidAtom;
				_NR<ASObject> proto;
				if (asAtomHandler::is<IFunction>(func))
				{
					proto = _MNR(asAtomHandler::as<IFunction>(func)->getprop_prototype());
					if (proto.isNull())
						proto = asAtomHandler::as<IFunction>(func)->prototype;
					else
						proto->incRef();
					if (!proto.isNull())
					{
						multiname mconstr(nullptr);
						mconstr.name_type=multiname::NAME_STRING;
						mconstr.name_s_id=BUILTIN_STRINGS::STRING_CONSTRUCTOR;
						mconstr.isAttribute = false;
						asAtom constr = asAtomHandler::invalidAtom;
						proto->getVariableByMultiname(constr,mconstr,GET_VARIABLE_OPTION::NONE,wrk);
						if (asAtomHandler::is<Class_base>(constr))
							asAtomHandler::as<Class_base>(constr)->getInstance(wrk,ret,true,nullptr,0);
						else if (proto->getClass())
							proto->getClass()->getInstance(wrk,ret,true,nullptr,0);
						asAtomHandler::toObject(ret,wrk)->setprop_prototype(proto);
						asAtomHandler::toObject(ret,wrk)->setprop_prototype(proto,BUILTIN_STRINGS::STRING_PROTO);
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
				if (context->exceptionthrown)
				{
					ret = asAtomHandler::undefinedAtom;
					context->exceptionthrown->decRef();
					context->exceptionthrown=nullptr;
				}
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
				PushStack(stack, asAtomHandler::fromBool(ABCVm::instanceOf(value,type)));
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
				PushStack(stack,asAtomHandler::fromBool(asAtomHandler::isLess(arg1,wrk,arg2) == TTRUE));
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
				clip->getSystemState()->openPageInBrowser(s1,s2);
				break;
			}
			case 0x87: // ActionStoreRegister
			{
				asAtom a = PeekStack(stack);
				ASATOM_INCREF(a);
				uint8_t num = *it++;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionStoreRegister "<<(int)num<<" "<<asAtomHandler::toDebugString(a));
				ASATOM_DECREF(registers[num]);
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionWaitForFrame "<<frame<<"/"<<clip->as<MovieClip>()->getFramesLoaded()<<" skip "<<skipcount);
				if (clip->as<MovieClip>()->getFramesLoaded() <= frame && !clip->as<MovieClip>()->hasFinishedLoading())
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
					clip = originalclip;
				else
				{
					auto newTarget = clip->AVM1GetClipFromPath(s);
					clip = newTarget != nullptr ? newTarget : clip->AVM1getRoot();
					if (newTarget == nullptr)
						LOG(LOG_ERROR,"AVM1: ActionSetTarget clip not found:"<<s);
				}
				clip->incRef();
				context->scope->setTargetScope(_MR(clip));
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
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionGotoLabel "<<s);
				clip->as<MovieClip>()->AVM1gotoFrameLabel(s,true,true);
				break;
			}
			case 0x8e: // ActionDefineFunction2
			{
				tiny_string name((const char*)&(*it),true);
				it += name.numBytes()+1;
				uint32_t paramcount = uint32_t(*it++) | ((*it++)<<8);
				*it++; //register count not used
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
				Activation_object* act = name == "" ? new_activationObject(wrk) : nullptr;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionDefineFunction2 "<<name<<" "<<paramcount<<" "<<flag1<<flag2<<flag3<<flag4<<flag5<<flag6<<flag7<<flag8<<flag9<<" "<<codesize<<" "<<act);
				AVM1Function* f = Class<IFunction>::getAVM1Function(wrk,clip,act,context,funcparamnames,code,context->scope,registernumber,flag1, flag2, flag3, flag4, flag5, flag6, flag7, flag8, flag9);
				//Create the prototype object
				f->prototype = _MR(new_asobject(f->getSystemState()->worker));
				f->prototype->addStoredMember();
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
				Log::calls_indent++;
				LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" ActionWith "<<codesize<<" "<<asAtomHandler::toDebugString(obj));
				++curdepth;
				scopestackstop[curdepth] = itend;
				context->scope = _MNR(new AVM1Scope
				(
					context->scope,
					_MR(asAtomHandler::toObject(obj, wrk))
				));
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
							uint32_t d=GUINT32_FROM_LE(*(uint32_t*)&(*it));
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
				Activation_object* act = name == "" ? new_activationObject(wrk) : nullptr;
				AVM1Function* f = Class<IFunction>::getAVM1Function(wrk,clip,act,context,paramnames,code,context->scope);
				//Create the prototype object
				f->prototype = _MR(new_asobject(f->getSystemState()->worker));
				f->prototype->addStoredMember();
				if (name == "")
				{
					asAtom a = asAtomHandler::fromObject(f);
					PushStack(stack,a);
				}
				else
					clip->AVM1SetFunction(name,_MR(f));
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
					clip->as<MovieClip>()->AVM1ExecuteFrameActions(frame);
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
					frame = clip->as<MovieClip>()->getFrameIdByLabel(s,"");
				}
				else
				{
					frame = asAtomHandler::toUInt(a);
				}
				if (clip->as<MovieClip>()->getFramesLoaded() <= frame && !clip->as<MovieClip>()->hasFinishedLoading())
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
			case 0x31: // ActionMBStringLength
			case 0x32: // ActionCharToAscii
			case 0x35: // ActionMBStringExtract
			case 0x36: // ActionMBCharToAscii
			case 0x37: // ActionMBAsciiToChar
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF4 DoActionTag "<<hex<<(int)opcode);
				if(opcode >= 0x80)
				{
					uint32_t len = ((*(it-1))<<8) | (*(it-2));
					it+=len;
				}
				break;
			case 0x45: // ActionTargetPath
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF5 DoActionTag "<<hex<<(int)opcode);
				break;
			case 0x68: // ActionStringGreater
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF6 DoActionTag "<<hex<<(int)opcode);
				break;
			case 0x2c: // ActionImplementsOp
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" SWF7 DoActionTag "<<hex<<(int)opcode);
				if(opcode >= 0x80)
				{
					uint32_t len = ((*(it-1))<<8) | (*(it-2));
					it+=len;
				}
				break;
			default:
				LOG(LOG_NOT_IMPLEMENTED,"AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" invalid DoActionTag "<<hex<<(int)opcode);
				throw RunTimeException("invalid AVM1 tag");
				break;
		}
		++context->actionsExecuted;
	}

	if (!scope.isNull() && isClosure)
	{
		scope->incRef();
		context->scope = scope;
	}
	else
	{
		context->scope.reset();
		context->globalScope.reset();
	}

	for (uint32_t i = 0; i < 256; i++)
	{
		ASATOM_DECREF(registers[i]);
	}
	while (!stack.empty())
	{
		asAtom a = PopStack(stack);
		LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" stack not empty:"<<asAtomHandler::toDebugString(a));
		ASATOM_DECREF(a);
	}
	if (clip_isTarget)
		clip->decRef();
	if (argsArray != nullptr)
		argsArray->decRef();
	LOG_CALL("AVM1:"<<clip->getTagID()<<" "<<(clip->is<MovieClip>() ? clip->as<MovieClip>()->state.FP : 0)<<" executeActions done");
	Log::calls_indent--;
	context->callDepth--;
	wrk->AVM1callStack.pop_back();
	if (context->exceptionthrown && context->callDepth==0)
	{
		if (wrk->AVM1callStack.empty())
		{
			LOG(LOG_ERROR,"AVM1: unhandled exception:"<<context->exceptionthrown->toDebugString());
			context->exceptionthrown->decRef();
		}
		else
			wrk->AVM1callStack.back()->exceptionthrown=context->exceptionthrown;
		context->exceptionthrown=nullptr;
	}
}
