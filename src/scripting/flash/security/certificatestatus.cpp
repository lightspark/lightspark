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

#include "scripting/flash/security/certificatestatus.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include <iomanip>

using namespace lightspark;

void CertificateStatus::sinit(Class_base* c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED|CLASS_FINAL);
	c->setVariableAtomByQName("EXPIRED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"expired"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVALID",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invalid"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("INVALID_CHAIN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"invalidChain"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("NOT_YET_VALID",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"notYetValid"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("PRINCIPAL_MISMATCH",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"principalMismatch"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("REVOKED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"revoked"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("TRUSTED",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"trusted"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNKNOWN",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"unknown"),CONSTANT_TRAIT);
	c->setVariableAtomByQName("UNTRUSTED_SIGNERS",nsNameAndKind(),asAtomHandler::fromString(c->getSystemState(),"untrustedSigners"),CONSTANT_TRAIT);
}
