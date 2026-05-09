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

#ifndef GC_CONTEXT_H
#define GC_CONTEXT_H 1

#include <cstddef>
#include <list>

// Loosely based on Gnash's `GC`.

namespace lightspark
{

template<typename T>
class GcPtr;
class GcResource;
class GcRoot;

// A garbage collector context.
//
// Instances of this class can manage a list of heap allocated pointers
// (resources), and delete them when they aren't needed/reachable.
//
// Their reachability is detected starting from a root resource, which
// in turn marks all reachable resources.
class GcContext
{
private:
	#ifdef GC_DEBUG
	// The number of times the collector has been run (useful for
	// stats/profiling).
	size_t collectorRuns;
	#endif

	// The number of newly added resources, since the last collection
	// cycle needed to trigger the next collection cycle.
	size_t maxNewResources;

	// A list of collectable resources.
	std::list<GcPtr<GcResource>> resources;

	// The root resource.
	GcRoot& root;

	// The number of resources in the resource list, at the end of the
	// last collection run.
	size_t lastResourceCount;

	// Mark all reachable resources.
	void markReachable();

	// Delete all unreachable resources, and mark all reachable resources
	// as unreachable.
	//
	// This returns the number of resources that were deleted.
	size_t cleanUnreachable();
public:
	// Creates a garbage collector, with the supplied root.
	//
	// `root` is the top level of the context, and takes care of marking
	// all further resources.
	// `collectThreshold` is the number of newly added resources since
	// the last collection cycle needed before the next collection cycle
	// occurs.
	GcContext(GcRoot& _root, size_t collectThreshold = 64);

	// Destroys the garbage collector, releasing all resources in the
	// process.
	~GcContext();

	// Adds a resource to the list of managed resources.
	//
	// The supplied resource isn't expected to already be in the list.
	// Failing to do would mainly degrade performance, but might not be
	// a problem.
	//
	// An assertion will fail if a resource is added twice.
	//
	// Preconditions:
	// - The resource isn't already in this collector's list.
	// - The resource is unreachable.
	// - The resource isn't managed by another context (this isn't
	// checked).
	//
	// `item` is the item to be managed by this collector.
	// NOTE: `item` is a `GcPtr`, meaning it can't be null.
	// NOTE: The caller gives up ownership of it, and will only be
	// deleted by this collector.
	void addResource(GcPtr<GcResource> item);

	// Run the collector, if it's worth it.
	//
	// Returns `true`, if the collector was ran.
	bool tryCollect();

	// Run the collection cycle.
	//
	// Find all reachable resources, and destroy the others.
	void runCollection();
};

}
#endif /* GC_CONTEXT_H */
