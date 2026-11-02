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

#include <initializer_list>

#include "gc/context.h"
#include "scripting/avm1/activation.h"
#include "scripting/avm1/prop_decl.h"
#include "scripting/avm1/toplevel/PrintJob.h"
#include "swf.h"

using namespace lightspark;

// Based on Ruffle's `avm1::globals::print_job` crate.

AVM1PrintJob::AVM1PrintJob(AVM1Activation& act) : AVM1Object
(
	act,
	act.getPrototypes().printJob->proto
) {}

constexpr auto protoFlags =
(
	AVM1PropFlags::DontDelete |
	AVM1PropFlags::ReadOnly
);

constexpr auto protoFlagsV7 = protoFlags | AVM1PropFlags::Version7;

using AVM1PrintJob;

static constexpr auto protoDecls =
{
	AVM1_FUNCTION_TYPE_PROTO(AVM1PrintJob, start, protoFlagsV7),
	AVM1_FUNCTION_TYPE_PROTO(AVM1PrintJob, addPage, protoFlagsV7),
	AVM1_FUNCTION_TYPE_PROTO(AVM1PrintJob, send, protoFlagsV7),
	AVM1_GETTER_TYPE_PROTO(AVM1PrintJob, PaperWidth, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1PrintJob, PaperHeight, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1PrintJob, PageWidth, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1PrintJob, PageHeight, protoFlags),
	AVM1_GETTER_TYPE_PROTO(AVM1PrintJob, Orientation, protoFlags)
};

GcPtr<AVM1SystemClass> AVM1PrintJob::createClass
(
	AVM1DeclContext& ctx,
	const GcPtr<AVM1Object>& superProto
)
{
	auto _class = ctx.makeClass(ctor, superProto);

	ctx.definePropsOn(_class->proto, protoDecls);
	return _class;
}

AVM1_FUNCTION_BODY(AVM1PrintJob, ctor)
{
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1PrintJob, AVM1PrintJob, start)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `PrintJob.start()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1PrintJob, AVM1PrintJob, addPage)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `PrintJob.addPage()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_FUNCTION_TYPE_BODY(AVM1PrintJob, AVM1PrintJob, send)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `PrintJob.send()` is a stub.");
	return AVM1Value::undefinedVal;
}

AVM1_GETTER_TYPE_BODY(AVM1PrintJob, AVM1PrintJob, PaperWidth)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `PrintJob.paperWidth` is a stub.");
	return 0;
}

AVM1_GETTER_TYPE_BODY(AVM1PrintJob, AVM1PrintJob, PaperHeight)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `PrintJob.paperHeight` is a stub.");
	return 0;
}

AVM1_GETTER_TYPE_BODY(AVM1PrintJob, AVM1PrintJob, PageWidth)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `PrintJob.pageWidth` is a stub.");
	return 0;
}

AVM1_GETTER_TYPE_BODY(AVM1PrintJob, AVM1PrintJob, PageHeight)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `PrintJob.pageHeight` is a stub.");
	return 0;
}

AVM1_GETTER_TYPE_BODY(AVM1PrintJob, AVM1PrintJob, Orientation)
{
	LOG(LOG_NOT_IMPLEMENTED, "AVM1: `PrintJob.orientation` is a stub.");
	return "portrait";
}
