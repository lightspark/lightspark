/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2026  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef SCRIPTING_AVM1_TOPLEVEL_PRINTJOB_H
#define SCRIPTING_AVM1_TOPLEVEL_PRINTJOB_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::globals::print_job` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1SystemClass;

class AVM1PrintJob : public AVM1Object
{
public:
	AVM1PrintJob(AVM1Activation& act);

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_DECL(ctor);

	AVM1_FUNCTION_TYPE_DECL(AVM1PrintJob, start);
	AVM1_FUNCTION_TYPE_DECL(AVM1PrintJob, addPage);
	AVM1_FUNCTION_TYPE_DECL(AVM1PrintJob, send);
	AVM1_GETTER_TYPE_DECL(AVM1PrintJob, PaperWidth);
	AVM1_GETTER_TYPE_DECL(AVM1PrintJob, PaperHeight);
	AVM1_GETTER_TYPE_DECL(AVM1PrintJob, PageWidth);
	AVM1_GETTER_TYPE_DECL(AVM1PrintJob, PageHeight);
	AVM1_GETTER_TYPE_DECL(AVM1PrintJob, Orientation);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_PRINTJOB_H */
