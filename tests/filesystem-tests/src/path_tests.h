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

#ifndef PATH_TESTS_H
#define PATH_TESTS_H 1

#include <libtest++/test_runner.h>

#include "macros.h"

using namespace lightspark;
using namespace libtestpp;

namespace lightspark
{
	class tiny_string;
	template<typename T>
	class Optional;
	class Path;
};

tiny_string iter(const Path& path);
tiny_string reverseIter(const Path& path);

TEST_CASE_DECL(Path, nativeSeparator);
#ifndef _WIN32
TEST_CASE_DECL(Path, hostRootNameSupport);
#endif
TEST_CASE_DECL(Path, ctorDtor);
TEST_CASE_DECL(Path, assign);
TEST_CASE_DECL(Path, append);
TEST_CASE_DECL(Path, concat);
TEST_CASE_DECL(Path, modifier);
TEST_CASE_DECL(Path, nativeObserve);
TEST_CASE_DECL(Path, genericObserve);
TEST_CASE_DECL(Path, compare);
TEST_CASE_DECL(Path, decompose);
TEST_CASE_DECL(Path, query);
TEST_CASE_DECL(Path, gen);
TEST_CASE_DECL(Path, iter);
TEST_CASE_DECL(Path, nonMember);
TEST_CASE_DECL(Path, iostream);

#endif /* PATH_TESTS_H */
