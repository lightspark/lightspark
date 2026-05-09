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

#ifndef GC_ROOT_H
#define GC_ROOT_H 1

// Loosely based on Gnash's `GcRoot`.

namespace lightspark
{

// A pure virtual class to allow the GC to store "roots", i.e. top level
// containers of `GcResource`s.
//
// Any class that are expected to act like a "root" for the purposes of
// garbage collection should inherit from this class, and implement
// `markReachableResources()`.
class GcRoot
{
public:
	// Scan all `GcResource`s that are reachable by this root.
	//
	// This function is called on roots that are owned by a `GcContext`.
	//
	// Use `setReachable()` on the resources stored in this root.
	virtual void markReachableResources() = 0;

	virtual ~GcRoot() {}
};

}
#endif /* GC_ROOT_H */
