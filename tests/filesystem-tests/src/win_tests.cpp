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

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <lightspark/tiny_string.h>
#include <lightspark/utils/filesystem.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/path.h>

#include <libtest++/test_runner.h>

#include "macros.h"
#include "win_tests.h"

using namespace lightspark;
using namespace libtestpp;

using OutcomeType = Outcome::Type;

TEST_CASE_DECL(Win, lfn)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(Win, pathNamespace)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(Win, mappedFolder)
{
	return Outcome(OutcomeType::Ignored);
}

TEST_CASE_DECL(Win, readOnlyRemove)
{
	return Outcome(OutcomeType::Ignored);
}
