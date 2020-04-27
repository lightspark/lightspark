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

#include "scripting/flash/display/shaderparametertype.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void ShaderParameterType::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED|CLASS_FINAL);
	c->setVariableAtomByQName("BOOL",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bool"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BOOL2",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bool2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BOOL3",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bool3"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("BOOL4",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"bool4"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"float"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT2",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"float2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT3",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"float3"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("FLOAT4",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"float4"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INT",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"int"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INT2",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"int2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INT3",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"int3"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INT4",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"int4"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MATRIX2X2",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"matrix2x2"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MATRIX3X3",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"matrix3x3"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("MATRIX4X4",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"matrix4x4"),CONSTANT_TRAIT);
}