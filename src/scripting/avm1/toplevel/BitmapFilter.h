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

#ifndef SCRIPTING_AVM1_TOPLEVEL_BITMAPFILTER_H
#define SCRIPTING_AVM1_TOPLEVEL_BITMAPFILTER_H 1

#include "gc/ptr.h"
#include "scripting/avm1/function.h"
#include "scripting/avm1/object/object.h"

// Based on Ruffle's `avm1::globals::bitmap_filter` crate.

namespace lightspark
{

class AVM1Activation;
class AVM1DeclContext;
class AVM1SystemClass;
class AVM1Value;
class FILTER;
class GcContext;

class AVM1BitmapFilter : public AVM1Object
{
private:
	virtual GcPtr<AVM1BitmapFilter> cloneImpl(AVM1Activation& act) const = 0;
	virtual FILTER toFilterImpl() const = 0;
public:
	AVM1BitmapFilter
	(
		GcContext& ctx,
		const GcPtr<AVM1Object>& superProto
	) : AVM1Object(ctx, superProto) {}

	operator FILTER() const { return toFilterImpl(); }

	static GcPtr<AVM1SystemClass> createClass
	(
		AVM1DeclContext& ctx,
		const GcPtr<AVM1Object>& superProto
	);

	AVM1_FUNCTION_TYPE_DECL(AVM1BitmapFilter, clone);
};

}
#endif /* SCRIPTING_AVM1_TOPLEVEL_BITMAPFILTER_H */
