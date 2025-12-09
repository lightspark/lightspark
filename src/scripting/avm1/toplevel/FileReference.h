/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2025  mr b0nk 500 (b0nk@b0nk.xyz)

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

#ifndef SCRIPTING_AVM1_TOPLEVEL_FILEREFERENCE_H
#define SCRIPTING_AVM1_TOPLEVEL_FILEREFERENCE_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"
#include "tiny_string.h"
#include "utils/optional.h"

// Based on Ruffle's `avm1::globals::file_reference` crate.

namespace lightspark
{

class AsBroadcasterFuncs;
class AVM1Activation;
class AVM1Date;
class AVM1DeclContext;
class AVM1SystemClass;

class AVM1FileReference : public AVM1Object
{
private:
	struct Info
	{
		NullableGcPtr<AVM1Date> creationDate;
		Optional<tiny_string> creator;
		NullableGcPtr<AVM1Date> modifyDate;
		Optional<tiny_string> name;
		Optional<size_t> size;
		Optional<tiny_string> fileType;
	};

	Optional<tiny_string> postData;
	Optional<Info> info;
public:
	AVM1FileReference(AVM1Activation& act) : AVM1Object(act) {}

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto,
		const GcPtr<AsBroadcasterFuncs>& bcastFuncs,
		const GcPtr<AVM1Object>& arrayProto
	);

	AVM1_FUNCTION_DECL(ctor);
	AVM1_GETTER_TYPE_DECL(AVM1FileReference, CreationDate);
	AVM1_GETTER_TYPE_DECL(AVM1FileReference, Creator);
	AVM1_GETTER_TYPE_DECL(AVM1FileReference, ModificationDate);
	AVM1_GETTER_TYPE_DECL(AVM1FileReference, Name);
	AVM1_PROPERTY_TYPE_DECL(AVM1FileReference, PostData);
	AVM1_GETTER_TYPE_DECL(AVM1FileReference, Size);
	AVM1_GETTER_TYPE_DECL(AVM1FileReference, Type);
	AVM1_FUNCTION_TYPE_DECL(AVM1FileReference, browse);
	AVM1_FUNCTION_TYPE_DECL(AVM1FileReference, cancel);
	AVM1_FUNCTION_TYPE_DECL(AVM1FileReference, download);
	AVM1_FUNCTION_TYPE_DECL(AVM1FileReference, upload);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_FILEREFERENCE_H */
