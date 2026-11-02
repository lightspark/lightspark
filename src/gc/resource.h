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

#ifndef GC_RESOURCE_H
#define GC_RESOURCE_H 1

// Loosely based on Gnash's `GcResource`.

namespace lightspark
{

class GcContext;

// A garbage collectable resource.
//
// Any instance of this class can be managed by a GC object.
class GcResource
{
	friend class GcContext;
private:
	bool reachable;
protected:
	// Scan all `GcResource`s reachable by this object.
	//
	// This function is called everytime this object goes from being
	// unreachable, to reachable, and is used to recursively mark all
	// contained resources as reachable.
	//
	// See `setReachable()` for more info.
	//
	// The default implementation doesn't mark anything.
	virtual void markReachableResources();

	// Delete this resource.
	//
	// NOTE: This is protected to allow for sub classing, but ideally,
	// it should be private, so that only `GcContext` is allowed to
	// delete it.
	virtual ~GcResource() {}
public:
	// Creates a garbage collected resource, that's associated with a
	// `GcContext`.
	GcResource(GcContext& ctx);

	// Marks this resource as being reachable.
	//
	// This can also lead to further marking of all resources that this
	// object can reach.
	//
	// If the object wasn't already marked as reachable, then this call
	// will trigger a scan of all contained objects as well.
	void setReachable();

	// Returns `true`, if this object is marked as reachable.
	bool isReachable() const { return reachable; }

	// Clears the reachable flag.
	void clearReachable() { reachable = false; }
};

}
#endif /* GC_RESOURCE_H */
