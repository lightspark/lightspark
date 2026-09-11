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

#include "gc/context.h"
#include "gc/resource.h"
#include "gc/root.h"
#include "gc/ptr.h"
#include "logger.h"

using namespace lightspark;

// Loosely based on Gnash's `GC`.

GcContext::GcContext(GcRoot& _root, size_t collectThreshold) :
#ifdef GC_DEBUG
collectorRuns(0),
#endif
maxNewResources(collectThreshold),
root(_root),
lastResourceCount(0)
{
	// TODO: Add a `LOG_DEBUG` log level for debugging purposes.
	LOG(LOG_TRACE, "GcContext " << this << " created.");
}

GcContext::~GcContext()
{
	// TODO: Add a `LOG_DEBUG` log level for debugging purposes.
	LOG
	(
		LOG_TRACE,
		"GcContext " << this << " has been deleted, "
		"deleting all managed resources."
		#ifdef GC_DEBUG
		" - It's collector ran " << collectorRuns << " times."
		#endif
	);

	for (const auto& res : resources)
		delete *res;
}

void GcContext::addResource(GcPtr<GcResource> item)
{
	assert(!item->isReachable());
	resources.emplace_back(item);

	// TODO: Add a `LOG_DEBUG` log level for debugging purposes.
	LOG
	(
		LOG_TRACE,
		"GcContext::addResource: "
		"Resource " << item << " was added. "
		"Total number of managed resources: " << resources.size()
	);
}

bool GcContext::tryCollect()
{
	// Notes on the heuristic to decide whether to run a collection
	// cycle, or not.
	//
	// Stuff to consider:
	//
	// Cost:
	// - Depends on the number of reachable reachable resources.
	// - Depends on the collection frequency.
	// Advantages:
	// - Depends on the number of unreachable resources.
	// Cheap to compute info:
	// - Total amount of heap allocated memory.
	//
	// The current heuristic:
	// - We run the collection cycle again, if *n* new resources were
	// added/allocated since the last cycle. *n* currently defaults to
	// `maxNewResources`.
	//
	// Potential improvements:
	// - Adapt `maxNewResources` dynamically, based on how long the
	// collection cycle took.
	// - Add refcounting support, like the AVM2 specific GC.
	//
	// NOTE: If we decide to implement the first one, we'd have to add
	// an option to disable it, for things that need to be deterministic,
	// such as the test runner, and TASing support, once that's added.
	auto size = resources.size();
	if (size >= lastResourceCount + maxNewResources)
	{
		runCollection();
		return true;
	}

	// TODO: Add a `LOG_DEBUG` log level for debugging purposes.
	LOG
	(
		LOG_TRACE,
		"GcContext::tryCollect: "
		"Collection cycle skipped - " <<
		size - lastResourceCount << '/' << maxNewResources << " "
		"new resources were added/allocated since the last cycle "
		"(from " << lastResourceCount << " to " << size << ")."
	);
	return false;
}

void GcContext::runCollection()
{
	#ifdef GC_DEBUG
	++collectionRuns;
	#endif

	auto size = resources.size();
	// TODO: Add a `LOG_DEBUG` log level for debugging purposes.
	LOG
	(
		LOG_TRACE,
		"GcContext::runCollection: "
		"Collection cycle started - " <<
		size - lastResourceCount << '/' << maxNewResources << " "
		"new resources were added/allocated since the last cycle "
		"(from " << lastResourceCount << " to " << size << ")."
	);

	// Mark all resources as reachable.
	markReachable();

	// Clean up any unreachable resources, and mark the reachable ones
	// as unreachable again.
	(void)cleanUnreachable();

	lastResourceCount = resources.size();
}

void GcContext::markReachable()
{
	// TODO: Add a `LOG_DEBUG` log level for debugging purposes.
	LOG
	(
		LOG_TRACE,
		"GcContext::markReachable: "
		"Context " << this << " started a mark scan on "
		"root " << &root << '.'
	);

	root.markReachableResources();
}

size_t GcContext::cleanUnreachable()
{
	auto prevSize = resources.size();

	// TODO: Add a `LOG_DEBUG` log level for debugging purposes.
	LOG
	(
		LOG_TRACE,
		"GcContext::cleanUnreachable: "
		"Context " << this << " started a sweep scan."
	);
	
	size_t deleted = 0;
	resources.remove_if([&](auto& res)
	{
		if (res->isReachable())
		{
			res->clearReachable();
			return false;
		}

		LOG
		(
			LOG_TRACE,
			"GcContext::cleanUnreachable: "
			"Recycling resource " << res << '.'
		);

		++deleted;
		delete *res;
		return true;
	});

	LOG
	(
		LOG_TRACE,
		"GcContext::cleanUnreachable: "
		"Recycled " << deleted << " unreachable resources "
		"(from " << prevSize << " to " << resources.size() << ")."
	);
}
