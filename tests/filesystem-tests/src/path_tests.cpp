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
#include <iostream>
#include <sstream>
#include <vector>

#include <lightspark/tiny_string.h>
#include <lightspark/utils/filesystem.h>
#include <lightspark/utils/optional.h>
#include <lightspark/utils/path.h>

#include <libtest++/test_runner.h>

#include "macros.h"
#include "path_tests.h"

using namespace lightspark;
using namespace libtestpp;

namespace fs = FileSystem;

using OutcomeType = Outcome::Type;
using ValueType = Path::ValueType;
using StringType = Path::StringType;

//#define USE_QUOTED_STRING

// TODO: Move this into `Path`.
std::ostream& operator<<(std::ostream& s, const Path& path)
{
	#ifdef USE_QUOTED_STRING
	s << '"';
	for (auto ch : path.getStr())
	{
		if (ch == '"' || ch == '\\')
			s << '\\';
		s << StringType::fromChar(ch);
	}
	return s << '"';
	#else
	const auto& str = path.getStr();
	return s.write(str.raw_buf(), str.numBytes());
	#endif
}

bool hasHostRootNameSupport()
{
	return Path("//host").hasRootName();
}

TEST_CASE_DECL(Path, nativeSeparator)
{
	#ifdef _WIN32
	REQUIRE_EQ_CAST(Path::nativeSeparator, '\\', char, char);
	#else
	REQUIRE_EQ_CAST(Path::nativeSeparator, '/', char, char);
	#endif
	return Outcome(OutcomeType::Passed);
}

#ifndef _WIN32
TEST_CASE_DECL(Path, hostRootNameSupport)
{
	if (hasHostRootNameSupport())
		return Outcome(OutcomeType::Passed);

	std::cerr << std::endl << "WARNING: " <<
	R"(`Path("//host").hasRootName() == true` )" <<
	"isn't supported on this platform, tests " <<
	"based on this are skipped (Which should be ok)." << std::endl;
	return Outcome(OutcomeType::Ignored);
}
#endif

TEST_CASE_DECL(Path, ctorDtor)
{
	std::stringstream s;
	CHECK_EQ(s, Path("/usr/local/bin").getGenericStr(), "/usr/local/bin");

	Path::StringType str = "/usr/local/bin";
	CHECK_EQ(s, Path(str, Path::Format::Generic), str);
	CHECK_EQ(s, Path(str.begin(), str.end()), str);

	CHECK_EQ(s, Path(std::string(3, 67)), "CCC");

	CHECK_EQ(s, Path("///foo/bar"), "/foo/bar");
	CHECK_EQ(s, Path("//foo//bar"), "//foo/bar");

	#ifdef _WIN32
	CHECK_EQ(s, Path("/usr/local/bin"), "\\usr\\local\\bin");
	CHECK_EQ(s, Path("C:\\usr\\local\\bin"), "C:\\usr\\local\\bin");
	#else
	CHECK_EQ(s, Path("/usr/local/bin"), "/usr/local/bin");
	#endif
	if (hasHostRootNameSupport())
		CHECK_EQ(s, Path("//host/foo/bar"), "//host/foo/bar");

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, assign)
{

	Path a("/foo/bar");
	Path b("/usr/local");
	Path c;

	c = a;
	REQUIRE_EQ(a, c);

	c = Path("/usr/local");
	REQUIRE_EQ(b, c);

	c = Path::StringType("/foo/bar");
	REQUIRE_EQ(a, c);

	c.assign(Path::StringType("/usr/local"));
	REQUIRE_EQ(b, c);

	Path::StringType str = "/foo/bar";
	c = Path(str.begin(), str.end());
	REQUIRE_EQ(a, c);

	str = "/usr/local";
	c.assign(str.begin(), str.end());
	REQUIRE_EQ(b, c);

	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, append)
{
	std::stringstream s;

	#ifdef _WIN32
	CHECK_EQ(s, Path("foo") / "c:/bar", "c:/bar");
	CHECK_EQ(s, Path("foo") / "c:", "c:");
	CHECK_EQ(s, Path("c:") / "", "c:");
	CHECK_EQ(s, Path("c:foo") / "/bar", "c:/bar");
	CHECK_EQ(s, Path("c:foo") / "c:bar", "c:foo/bar");
	#else
	CHECK_EQ(s, Path("foo") / "", "foo/");
	CHECK_EQ(s, Path("foo") / "/bar", "/bar");
	CHECK_EQ(s, Path("/foo") / "/", "/");

	if (hasHostRootNameSupport())
	{
		CHECK_EQ(s, Path("//host/foo") / "/bar", "/bar");
		CHECK_EQ(s, Path("//host") / "/", "//host/");
		CHECK_EQ(s, Path("//host/foo") / "/", "/");
	}
	#endif

	CHECK_EQ(s, Path("/foo/bar") / "some///other", "/foo/bar/some/other");

	Path a("/tmp/test");
	Path b("foobar.txt");
	Path c = a / b;

	CHECK_EQ(s, c, "/tmp/test/foobar.txt");

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, concat)
{
	std::stringstream s;

	CHECK_EQ(s, (Path("foo") += Path("bar")), "foobar");
	CHECK_EQ(s, (Path("foo") += Path("/bar")), "foo/bar");

	CHECK_EQ(s, (Path("foo") += Path::StringType("bar")), "foobar");
	CHECK_EQ(s, (Path("foo") += Path::StringType("/bar")), "foo/bar");

	CHECK_EQ(s, (Path("foo") += "bar"), "foobar");
	CHECK_EQ(s, (Path("foo") += "/bar"), "foo/bar");

	CHECK_EQ(s, (Path("foo") += 'b'), "foob");
	CHECK_EQ(s, (Path("foo") += '/'), "foo/");

	CHECK_EQ(s, Path("foo").concat("bar"), "foobar");
	CHECK_EQ(s, Path("foo").concat("/bar"), "foo/bar");

	Path::StringType str = "bar";
	CHECK_EQ(s, Path("foo").concat(str.begin(), str.end()), "foobar");

	CHECK_EQ(s, (Path("/foo/bar") += "/some///other"), "/foo/bar/some/other");

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, modifier)
{
	std::stringstream s;

	Path path("/foo/bar");
	path.clear();

	CHECK_EQ(s, path, "");

	// NOTE: `makeNative()` is a no-op.
	#ifdef _WIN32
	CHECK_EQ(s, Path("foo\\bar"), "foo/bar");
	CHECK_EQ(s, Path("foo\\bar").makeNative(), "foo/bar");
	#else
	CHECK_EQ(s, Path("foo\\bar"), "foo\\bar");
	CHECK_EQ(s, Path("foo\\bar").makeNative(), "foo\\bar");
	#endif
	CHECK_EQ(s, Path("foo/bar").makeNative(), "foo/bar");

	CHECK_EQ(s, Path("foo/").removeFilename(), "foo/");
	CHECK_EQ(s, Path("/foo").removeFilename(), "/");
	CHECK_EQ(s, Path("/").removeFilename(), "/");

	CHECK_EQ(s, Path("/foo").replaceFilename("bar"), "/bar");
	CHECK_EQ(s, Path("/").replaceFilename("bar"), "/bar");
	CHECK_EQ(s, Path("/foo").replaceFilename("b//ar"), "/b/ar");

	CHECK_EQ(s, Path("/foo/bar.txt").replaceExtension("odf"), "/foo/bar.odf");
	CHECK_EQ(s, Path("/foo/bar.txt").replaceExtension(), "/foo/bar");
	CHECK_EQ(s, Path("/foo/bar").replaceExtension("odf"), "/foo/bar.odf");
	CHECK_EQ(s, Path("/foo/bar").replaceExtension(".odf"), "/foo/bar.odf");
	CHECK_EQ(s, Path("/foo/bar.").replaceExtension(".odf"), "/foo/bar.odf");
	CHECK_EQ(s, Path("/foo/bar/").replaceExtension("odf"), "/foo/bar/.odf");
	
	Path a("foo");
	Path b("bar");

	a.swap(b);

	CHECK_EQ(s, a, "bar");
	CHECK_EQ(s, b, "foo");

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, nativeObserve)
{
	std::stringstream s;

	#ifdef _WIN32
	CHECK_EQ(s, Path("\xc3\xa4\\\xe2\x82\xac").getNativeStr(), Path::StringType("\xc3\xa4\\\xe2\x82\xac"));
	CHECK_EQ(s, Path("\xc3\xa4\\\xe2\x82\xac").getStr(), Path::StringType(u8"\u00E4\\\u20AC"));
	CHECK_NOT_FUNC(s, Path("\xc3\xa4\\\xe2\x82\xac").rawBuf(), u8"ä\\€", strcmp);
	CHECK_EQ(s, Path("\xc3\xa4\\\xe2\x82\xac"), Path::StringType(u8"ä\\€"));
	#else
	CHECK_EQ(s, Path("\xc3\xa4/\xe2\x82\xac").getNativeStr(), Path::StringType("\xc3\xa4/\xe2\x82\xac"));
	CHECK_EQ(s, Path("\xc3\xa4/\xe2\x82\xac").getStr(), Path::StringType(u8"\u00E4/\u20AC"));
	CHECK_NOT_FUNC(s, Path("\xc3\xa4/\xe2\x82\xac").rawBuf(), u8"ä/€", strcmp);
	CHECK_EQ(s, Path("\xc3\xa4/\xe2\x82\xac"), Path::StringType(u8"ä/€"));
	#endif

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, genericObserve)
{
	std::stringstream s;

	#ifdef _WIN32
	auto str = Path("\xc3\xa4\\\xe2\x82\xac").getGenericStr();
	CHECK_EQ(s, str.raw_buf(), std::string("\xc3\xa4/\xe2\x82\xac"));
	CHECK_EQ(s, str, Path::StringType(u8"\u00E4/\u20AC"));
	CHECK_EQ(s, str, Path::StringType(u8"ä/€"));
	#else
	auto str = Path("\xc3\xa4/\xe2\x82\xac").getGenericStr();
	CHECK_EQ(s, str.raw_buf(), std::string("\xc3\xa4/\xe2\x82\xac"));
	CHECK_EQ(s, str, Path::StringType(u8"\u00E4/\u20AC"));
	CHECK_EQ(s, str, Path::StringType(u8"ä/€"));
	#endif

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, compare)
{
	std::stringstream s;

	CHECK_GT(s, Path("/foo/b").compare("/foo/a"), 0);
	CHECK_EQ(s, Path("/foo/b").compare("/foo/b"), 0);
	CHECK_LT(s, Path("/foo/b").compare("/foo/c"), 0);

	CHECK_GT(s, Path("/foo/b").compare(Path::StringType("/foo/a")), 0);
	CHECK_EQ(s, Path("/foo/b").compare(Path::StringType("/foo/b")), 0);
	CHECK_LT(s, Path("/foo/b").compare(Path::StringType("/foo/c")), 0);

	CHECK_GT(s, Path("/foo/b").compare(Path("/foo/a")), 0);
	CHECK_EQ(s, Path("/foo/b").compare(Path("/foo/b")), 0);
	CHECK_LT(s, Path("/foo/b").compare(Path("/foo/c")), 0);

	#ifdef _WIN32
	CHECK_EQ(s, Path("c:\\a\\b").compare("C:\\a\\b"), 0);
	CHECK_NE(s, Path("c:\\a\\b").compare("d:\\a\\b"), 0);
	CHECK_NE(s, Path("c:\\a\\b").compare("C:\\A\\b"), 0);
	#endif

	#ifdef USE_LWG_2936
	CHECK_LT(s, Path("/a/b/").compare("/a/b/c"), 0);
	CHECK_GT(s, Path("/a/b/").compare("a/c"), 0);
	#endif

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, decompose)
{
	std::stringstream s;

	// `getRootName()`
	CHECK_EQ(s, Path("").getRootName(), "");
	CHECK_EQ(s, Path(".").getRootName(), "");
	CHECK_EQ(s, Path("..").getRootName(), "");
	CHECK_EQ(s, Path("foo").getRootName(), "");
	CHECK_EQ(s, Path("/").getRootName(), "");
	CHECK_EQ(s, Path("/foo").getRootName(), "");
	CHECK_EQ(s, Path("foo/").getRootName(), "");
	CHECK_EQ(s, Path("/foo/").getRootName(), "");
	CHECK_EQ(s, Path("foo/bar").getRootName(), "");
	CHECK_EQ(s, Path("/foo/bar").getRootName(), "");
	CHECK_EQ(s, Path("///foo/bar").getRootName(), "");

	#ifdef _WIN32
	CHECK_EQ(s, Path("C:/foo").getRootName(), "C:");
	CHECK_EQ(s, Path("C:\\foo").getRootName(), "C:");
	CHECK_EQ(s, Path("C:foo").getRootName(), "C:");
	#endif

	// `getRootDir()`
	CHECK_EQ(s, Path("").getRootDir(), "");
	CHECK_EQ(s, Path(".").getRootDir(), "");
	CHECK_EQ(s, Path("..").getRootDir(), "");
	CHECK_EQ(s, Path("foo").getRootDir(), "");
	CHECK_EQ(s, Path("/").getRootDir(), "/");
	CHECK_EQ(s, Path("/foo").getRootDir(), "/");
	CHECK_EQ(s, Path("foo/").getRootDir(), "");
	CHECK_EQ(s, Path("/foo/").getRootDir(), "/");
	CHECK_EQ(s, Path("foo/bar").getRootDir(), "");
	CHECK_EQ(s, Path("/foo/bar").getRootDir(), "/");
	CHECK_EQ(s, Path("///foo/bar").getRootDir(), "/");

	#ifdef _WIN32
	CHECK_EQ(s, Path("C:/foo").getRootDir(), "/");
	CHECK_EQ(s, Path("C:\\foo").getRootDir(), "/");
	CHECK_EQ(s, Path("C:foo").getRootDir(), "");
	#endif

	// `getRoot()`
	CHECK_EQ(s, Path("").getRoot(), "");
	CHECK_EQ(s, Path(".").getRoot(), "");
	CHECK_EQ(s, Path("..").getRoot(), "");
	CHECK_EQ(s, Path("foo").getRoot(), "");
	CHECK_EQ(s, Path("/").getRoot(), "/");
	CHECK_EQ(s, Path("/foo").getRoot(), "/");
	CHECK_EQ(s, Path("foo/").getRoot(), "");
	CHECK_EQ(s, Path("/foo/").getRoot(), "/");
	CHECK_EQ(s, Path("foo/bar").getRoot(), "");
	CHECK_EQ(s, Path("/foo/bar").getRoot(), "/");
	CHECK_EQ(s, Path("///foo/bar").getRoot(), "/");

	#ifdef _WIN32
	CHECK_EQ(s, Path("C:/foo").getRoot(), "C:/");
	CHECK_EQ(s, Path("C:\\foo").getRoot(), "C:/");
	CHECK_EQ(s, Path("C:foo").getRoot(), "C:");
	#endif

	// `getRelative()`
	CHECK_EQ(s, Path("").getRelative(), "");
	CHECK_EQ(s, Path(".").getRelative(), ".");
	CHECK_EQ(s, Path("..").getRelative(), "..");
	CHECK_EQ(s, Path("foo").getRelative(), "foo");
	CHECK_EQ(s, Path("/").getRelative(), "");
	CHECK_EQ(s, Path("/foo").getRelative(), "foo");
	CHECK_EQ(s, Path("foo/").getRelative(), "foo/");
	CHECK_EQ(s, Path("/foo/").getRelative(), "foo/");
	CHECK_EQ(s, Path("foo/bar").getRelative(), "foo/bar");
	CHECK_EQ(s, Path("/foo/bar").getRelative(), "foo/bar");
	CHECK_EQ(s, Path("///foo/bar").getRelative(), "foo/bar");

	#ifdef _WIN32
	CHECK_EQ(s, Path("C:/foo").getRelative(), "foo");
	CHECK_EQ(s, Path("C:\\foo").getRelative(), "foo");
	CHECK_EQ(s, Path("C:foo").getRelative(), "foo");
	#endif

	// `getParent()`
	CHECK_EQ(s, Path("").getParent(), "");
	CHECK_EQ(s, Path(".").getParent(), "");
	// NOTE: This looks odd, but it's part of the spec.
	CHECK_EQ(s, Path("..").getParent(), "");
	CHECK_EQ(s, Path("foo").getParent(), "");
	CHECK_EQ(s, Path("/").getParent(), "/");
	CHECK_EQ(s, Path("/foo").getParent(), "/");
	CHECK_EQ(s, Path("foo/").getParent(), "foo");
	CHECK_EQ(s, Path("/foo/").getParent(), "/foo");
	CHECK_EQ(s, Path("foo/bar").getParent(), "foo");
	CHECK_EQ(s, Path("/foo/bar").getParent(), "/foo");
	CHECK_EQ(s, Path("///foo/bar").getParent(), "/foo");

	#ifdef _WIN32
	CHECK_EQ(s, Path("C:/foo").getParent(), "C:/");
	CHECK_EQ(s, Path("C:\\foo").getParent(), "C:/");
	CHECK_EQ(s, Path("C:foo").getParent(), "C:");
	#endif

	// `getFilename()`
	CHECK_EQ(s, Path("").getFilename(), "");
	CHECK_EQ(s, Path(".").getFilename(), ".");
	CHECK_EQ(s, Path("..").getFilename(), "..");
	CHECK_EQ(s, Path("foo").getFilename(), "foo");
	CHECK_EQ(s, Path("/").getFilename(), "");
	CHECK_EQ(s, Path("/foo").getFilename(), "foo");
	CHECK_EQ(s, Path("foo/").getFilename(), "");
	CHECK_EQ(s, Path("/foo/").getFilename(), "");
	CHECK_EQ(s, Path("foo/bar").getFilename(), "bar");
	CHECK_EQ(s, Path("/foo/bar").getFilename(), "bar");
	CHECK_EQ(s, Path("///foo/bar").getFilename(), "bar");

	#ifdef _WIN32
	CHECK_EQ(s, Path("C:/foo").getFilename(), "foo");
	CHECK_EQ(s, Path("C:\\foo").getFilename(), "foo");
	CHECK_EQ(s, Path("C:foo").getFilename(), "foo");
	CHECK_EQ(s, Path("t:est.txt").getFilename(), "est.txt");
	#else
	CHECK_EQ(s, Path("t:est.txt").getFilename(), "t:est.txt");
	#endif

	// `getStem()`
	CHECK_EQ(s, Path("/foo/bar.txt").getStem(), "bar");
	{
		Path path("foo.bar.baz.tar");
		CHECK_EQ(s, path.getExtension(), ".tar");
		path = path.getStem();
		CHECK_EQ(s, path.getExtension(), ".baz");
		path = path.getStem();
		CHECK_EQ(s, path.getExtension(), ".bar");
		path = path.getStem();
		CHECK_EQ(s, path.getExtension(), "foo");
	}

	CHECK_EQ(s, Path("/foo/.profile").getStem(), ".profile");
	CHECK_EQ(s, Path(".bar").getStem(), ".bar");
	CHECK_EQ(s, Path("..bar").getStem(), ".");

	#ifdef _WIN32
	CHECK_EQ(s, Path("t:est.txt").getStem(), "est");
	#else
	CHECK_EQ(s, Path("t:est.txt").getStem(), "t:est");
	#endif

	CHECK_EQ(s, Path("/foo/.").getStem(), ".");
	CHECK_EQ(s, Path("/foo/..").getStem(), "..");

	// `getExtension`()
	CHECK_EQ(s, Path("/foo/bar.txt").getExtension(), ".txt");
	CHECK_EQ(s, Path("/foo/bar").getExtension(), "");
	CHECK_EQ(s, Path("/foo/.profile").getExtension(), "");
	CHECK_EQ(s, Path(".bar").getExtension(), "");
	CHECK_EQ(s, Path("..bar").getExtension(), ".bar");
	CHECK_EQ(s, Path("t:est.txt").getExtension(), ".txt");
	CHECK_EQ(s, Path("/foo/.").getExtension(), "");
	CHECK_EQ(s, Path("/foo/..").getExtension(), "");

	// `//host` based root names.
	if (hasHostRootNameSupport())
	{
		CHECK_EQ(s, Path("//host").getRootName(), "//host");
		CHECK_EQ(s, Path("//host/foo").getRootName(), "//host");
		CHECK_EQ(s, Path("//host").getRootDir(), "");
		CHECK_EQ(s, Path("//host/foo").getRootDir(), "/");
		CHECK_EQ(s, Path("//host").getRoot(), "//host");
		CHECK_EQ(s, Path("//host/foo").getRoot(), "//host/");
		CHECK_EQ(s, Path("//host").getRelative(), "");
		CHECK_EQ(s, Path("//host/foo").getRelative(), "foo");
		CHECK_EQ(s, Path("//host").getParent(), "//host");
		CHECK_EQ(s, Path("//host/foo").getParent(), "//host/");
		CHECK_EQ(s, Path("//host").getFilename(), "");
		CHECK_EQ(s, Path("//host/foo").getFilename(), "foo");
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, query)
{
	std::stringstream s;

	// `empty()`
	CHECK_EQ(s, Path("").empty(), true);
	CHECK_EQ(s, Path("foo").empty(), false);

	// `hasRoot()`
	CHECK_EQ(s, Path("foo").hasRoot(), false);
	CHECK_EQ(s, Path("foo/bar").hasRoot(), false);
	CHECK_EQ(s, Path("/foo").hasRoot(), true);
	#ifdef _WIN32
	CHECK_EQ(s, Path("C:foo").hasRoot(), true);
	CHECK_EQ(s, Path("C:/foo").hasRoot(), true);
	#endif

	// `hasRootName()`
	CHECK_EQ(s, Path("foo").hasRootName(), false);
	CHECK_EQ(s, Path("foo/bar").hasRootName(), false);
	CHECK_EQ(s, Path("/foo").hasRootName(), false);
	#ifdef _WIN32
	CHECK_EQ(s, Path("C:foo").hasRootName(), true);
	CHECK_EQ(s, Path("C:/foo").hasRootName(), true);
	#endif

	// `hasRootDir()`
	CHECK_EQ(s, Path("foo").hasRootDir(), false);
	CHECK_EQ(s, Path("foo/bar").hasRootDir(), false);
	CHECK_EQ(s, Path("/foo").hasRootDir(), true);
	#ifdef _WIN32
	CHECK_EQ(s, Path("C:foo").hasRootDir(), false);
	CHECK_EQ(s, Path("C:/foo").hasRootDir(), true);
	#endif

	// `hasRelativePath()`
	CHECK_EQ(s, Path("").hasRelativePath(), false);
	CHECK_EQ(s, Path("/").hasRelativePath(), false);
	CHECK_EQ(s, Path("/foo").hasRelativePath(), true);

	// `hasParent()`
	CHECK_EQ(s, Path("").hasParent(), false);
	CHECK_EQ(s, Path(".").hasParent(), false);
	// NOTE: This looks odd, but it's part of the spec.
	CHECK_EQ(s, Path("..").hasParent(), false);
	CHECK_EQ(s, Path("foo").hasParent(), false);
	CHECK_EQ(s, Path("/").hasParent(), true);
	CHECK_EQ(s, Path("/foo").hasParent(), true);
	CHECK_EQ(s, Path("foo/").hasParent(), true);
	CHECK_EQ(s, Path("/foo/").hasParent(), true);

	// `hasFilename()`
	CHECK_EQ(s, Path("foo").hasFilename(), true);
	CHECK_EQ(s, Path("foo/bar").hasFilename(), true);
	CHECK_EQ(s, Path("/foo/bar/").hasFilename(), false);

	// `hasStem()`
	CHECK_EQ(s, Path("foo").hasStem(), true);
	CHECK_EQ(s, Path("foo.bar").hasStem(), true);
	CHECK_EQ(s, Path(".profile").hasStem(), true);
	CHECK_EQ(s, Path("/foo/").hasStem(), false);

	// `hasExtension()`
	CHECK_EQ(s, Path("foo").hasExtension(), false);
	CHECK_EQ(s, Path("foo.bar").hasExtension(), true);
	CHECK_EQ(s, Path(".profile").hasExtension(), false);

	// `isAbsolute()`
	CHECK_EQ(s, Path("foo/bar").isAbsolute(), false);
	#ifdef _WIN32
	CHECK_EQ(s, Path("/foo").isAbsolute(), false);
	CHECK_EQ(s, Path("C:foo").isAbsolute(), false);
	CHECK_EQ(s, Path("C:/foo").isAbsolute(), true);
	#else
	CHECK_EQ(s, Path("/foo").isAbsolute(), true);
	#endif

	// `isRelative()`
	CHECK_EQ(s, Path("foo/bar").isRelative(), true);
	#ifdef _WIN32
	CHECK_EQ(s, Path("/foo").isRelative(), true);
	CHECK_EQ(s, Path("C:foo").isRelative(), true);
	CHECK_EQ(s, Path("C:/foo").isRelative(), false);
	#else
	CHECK_EQ(s, Path("/foo").isRelative(), false);
	#endif

	if (hasHostRootNameSupport())
	{
		CHECK_EQ(s, Path("//host").hasRootName(), true);
		CHECK_EQ(s, Path("//host/foo").hasRootName(), true);
		CHECK_EQ(s, Path("//host").hasRoot(), true);
		CHECK_EQ(s, Path("//host/foo").hasRoot(), true);
		CHECK_EQ(s, Path("//host").hasRootDir(), false);
		CHECK_EQ(s, Path("//host/foo").hasRootDir(), true);
		CHECK_EQ(s, Path("//host").hasRelativePath(), false);
		CHECK_EQ(s, Path("//host/foo").hasRelativePath(), true);
		CHECK_EQ(s, Path("//host/foo").isAbsolute(), true);
		CHECK_EQ(s, Path("//host/foo").isRelative(), false);
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, gen)
{
	std::stringstream s;

	// `lexicallyNormal()`
	CHECK_EQ(s, Path("foo/./bar/..").lexicallyNormal(), "foo/");
	CHECK_EQ(s, Path("foo/.///bar/../").lexicallyNormal(), "foo/");
	CHECK_EQ(s, Path("/foo/../..").lexicallyNormal(), "/");
	CHECK_EQ(s, Path("foo/..").lexicallyNormal(), ".");
	CHECK_EQ(s, Path("ab/cd/ef/../../qw").lexicallyNormal(), "ab/qw");
	CHECK_EQ(s, Path("a/b/../../../c").lexicallyNormal(), "../c");
	CHECK_EQ(s, Path("../").lexicallyNormal(), "..");
	#ifdef _WIN32
	CHECK_EQ(s, Path("\\/\\///\\/").lexicallyNormal(), "/");
	CHECK_EQ(s, Path("a/b/..\\//..///\\/../c\\\\/").lexicallyNormal(), "../c/");
	CHECK_EQ(s, Path("..a/b/..\\//..///\\/../c\\\\/").lexicallyNormal(), "../c/");
	CHECK_EQ(s, Path("..\\").lexicallyNormal(), "..");
	#endif

	// `lexicallyRelative()`
	CHECK_EQ(s, Path("/a/d").lexicallyRelative("/a/b/c"), "../../d");
	CHECK_EQ(s, Path("/a/b/c").lexicallyRelative("/a/d"), "../b/c");
	CHECK_EQ(s, Path("/a/b/c").lexicallyRelative("/a/b/c/d/.."), ".");
	CHECK_EQ(s, Path("/a/b/c/").lexicallyRelative("/a/b/c/d/.."), ".");
	CHECK_EQ(s, Path("").lexicallyRelative("/a/.."), "");
	CHECK_EQ(s, Path("").lexicallyRelative("a/.."), ".");
	CHECK_EQ(s, Path("a/b/c").lexicallyRelative("a"), "b/c");
	CHECK_EQ(s, Path("a/b/c").lexicallyRelative("a/b/c/x/y"), "../..");
	CHECK_EQ(s, Path("a/b/c").lexicallyRelative("a/b/c"), ".");
	CHECK_EQ(s, Path("a/b").lexicallyRelative("c/d"), "../../a/b");
	CHECK_EQ(s, Path("a/b").lexicallyRelative("a/"), "b");
	if (hasHostRootNameSupport())
		CHECK_EQ(s, Path("//host1/foo").lexicallyRelative("//host2.bar"), "");
	#ifdef _WIN32
	CHECK_EQ(s, Path("c:/foo").lexicallyRelative("/bar"), "");
	CHECK_EQ(s, Path("c:foo").lexicallyRelative("c:/bar"), "");
	CHECK_EQ(s, Path("foo").lexicallyRelative("/bar"), "");
	CHECK_EQ(s, Path("c:/foo/bar.txt").lexicallyRelative("c:/foo/"), "bar.txt");
	CHECK_EQ(s, Path("c:/foo/bar.txt").lexicallyRelative("C:/foo/"), "bar.txt");
	#else
	CHECK_EQ(s, Path("/foo").lexicallyRelative("bar"), "");
	CHECK_EQ(s, Path("foo").lexicallyRelative("/bar"), "");
	#endif

	// `lexicallyProximate()`
	CHECK_EQ(s, Path("/a/d").lexicallyProximate("/a/b/c"), "../../d");
	if (hasHostRootNameSupport())
		CHECK_EQ(s, Path("//host1/a/d").lexicallyProximate("//host2/a/b/c"), "//host1/a/d");
	CHECK_EQ(s, Path("a/d").lexicallyProximate("/a/b/c"), "a/d");
	#ifdef _WIN32
	CHECK_EQ(s, Path("c:/a/d").lexicallyProximate("c:/a/b/c"), "../../d");
	CHECK_EQ(s, Path("c:/a/d").lexicallyProximate("d:/a/b/c"), "c:/a/d");
	CHECK_EQ(s, Path("c:/foo").lexicallyProximate("/bar"), "c:/foo");
	CHECK_EQ(s, Path("c:foo").lexicallyProximate("c:/bar"), "c:foo");
	CHECK_EQ(s, Path("foo").lexicallyProximate("/bar"), "foo");
	#else
	CHECK_EQ(s, Path("/foo").lexicallyProximate("bar"), "/foo");
	CHECK_EQ(s, Path("foo").lexicallyProximate("/bar"), "foo");
	#endif

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

static StringType iter(const Path& path)
{
	std::stringstream s;
	s << path.begin()->getGenericStr();
	for (auto it = std::next(path.begin()); it != path.end(); ++it)
		s << ',' << it->getGenericStr();
	return s.str();
}

static StringType reverseIter(const Path& path)
{
	std::stringstream s;
	auto it = path.end();
	s << (--it)->getGenericStr();

	if (it == path.begin())
		return s.str();

	while (it != path.begin())
		s << ',' << (--it)->getGenericStr();

	return s.str();
}

TEST_CASE_DECL(Path, iter)
{
	#ifdef _WIN32
	#else
	#endif
	#ifdef USE_LWG_2936
	#else
	#endif
	std::stringstream s;

	CHECK_EQ(s, iter(Path()).empty(), true);
	CHECK_EQ(s, iter(Path(".")), ".");
	CHECK_EQ(s, iter(Path("..")), "..");
	CHECK_EQ(s, iter(Path("foo")), "foo");
	CHECK_EQ(s, iter(Path("/")), "/");
	CHECK_EQ(s, iter(Path("/foo")), "/,foo");
	CHECK_EQ(s, iter(Path("foo/")), "foo,");
	CHECK_EQ(s, iter(Path("/foo/")), "/,foo,");
	CHECK_EQ(s, iter(Path("foo/bar")), "foo,bar");
	CHECK_EQ(s, iter(Path("/foo/bar")), "/,foo,bar");

	// NOTE: Because this implementation is based on `ghc::filesystem`,
	// it inherets the fact that it reduces duplicate leading separators
	// into a single separator. Usually, `std::filesystem` keeps duplicate
	// leading separators.
	CHECK_EQ(s, iter(Path("///foo/bar")), "/,foo,bar");

	CHECK_EQ(s, iter(Path("/foo/bar///")), "/,foo,bar,");
	CHECK_EQ(s, iter(Path("foo/.///bar/../")), "foo,.,bar,..,");
	#ifdef _WIN32
	CHECK_EQ(s, iter(Path("C:/foo")), "C:,/,foo");
	CHECK_EQ(s, iter(Path("C:foo")), "C:,foo");
	#endif

	CHECK_EQ(s, reverseIter(Path()).empty(), true);
	CHECK_EQ(s, reverseIter(Path(".")), ".");
	CHECK_EQ(s, reverseIter(Path("..")), "..");
	CHECK_EQ(s, reverseIter(Path("foo")), "foo");
	CHECK_EQ(s, reverseIter(Path("/")), "/");
	CHECK_EQ(s, reverseIter(Path("/foo")), "foo,/");
	CHECK_EQ(s, reverseIter(Path("foo/")), ",foo");
	CHECK_EQ(s, reverseIter(Path("/foo/")), ",foo,/");
	CHECK_EQ(s, reverseIter(Path("foo/bar")), "bar,foo");
	CHECK_EQ(s, reverseIter(Path("/foo/bar")), "bar,foo,/");

	// NOTE: Because this implementation is based on `ghc::filesystem`,
	// it inherets the fact that it reduces duplicate leading separators
	// into a single separator. Usually, `std::filesystem` keeps duplicate
	// leading separators.
	CHECK_EQ(s, reverseIter(Path("///foo/bar")), "bar,foo,/");

	CHECK_EQ(s, reverseIter(Path("/foo/bar///")), ",bar,foo,/");
	CHECK_EQ(s, reverseIter(Path("foo/.///bar/../")), ",..,bar,.,foo");
	#ifdef _WIN32
	CHECK_EQ(s, reverseIter(Path("C:/foo")), "foo,/,C:");
	CHECK_EQ(s, reverseIter(Path("C:foo")), "foo,C:");
	#endif

	{
		Path a("/foo/bar/test.txt");
		Path b;
		for (auto it : a)
		    b /= it;

		CHECK_EQ(s, a, b);
		CHECK_EQ(s, *--Path("/foo/bar").end(), "bar");
		auto c = Path("/foo/bar");
		auto it = c.end();
		it--;
		CHECK_EQ(s, *it, "bar");
	}
	
	if (hasHostRootNameSupport())
	{
		CHECK_EQ(s, *--Path("//host/foo").end(), "foo");
		auto a = Path("//host/foo");
		auto it = a.end();
		it--;
		CHECK_EQ(s, *it, "foo");
		CHECK_EQ(s, iter(Path("//host")), "//host");
		CHECK_EQ(s, iter(Path("//host/foo")), "//host,/,foo");
		CHECK_EQ(s, reverseIter(Path("//host")), "//host");
		CHECK_EQ(s, reverseIter(Path("//host/foo")), "foo,/,//host");
		{
			Path a = "//host/foo/bar/test.txt";
			Path b;
			for (auto it : a)
				b /= it;

			CHECK_EQ(s, b, a);
		}
	}

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

TEST_CASE_DECL(Path, nonMember)
{
	std::stringstream s;

	Path a("foo/bar");
	Path b("some/other");
	// TODO: Add `FileSystem::swap()`.
	//fs::swap(a, b);
	a.swap(b);
	CHECK_EQ(s, a, "some/other");
	CHECK_EQ(s, b, "foo/bar");
	// TODO: Add `FileSystem::hashValue()`.
	//CHECK_EQ(s, fs::hashValue(a), true);
	CHECK_BIN(s, b, a, <);
	CHECK_BIN(s, b, a, <=);
	CHECK_BIN(s, a, a, <=);
	CHECK_NOT_BIN(s, a, b, <);
	CHECK_NOT_BIN(s, a, b, <=);
	CHECK_BIN(s, a, b, >);
	CHECK_BIN(s, a, b, >=);
	CHECK_BIN(s, a, a, >=);
	CHECK_NOT_BIN(s, b, a, >);
	CHECK_NOT_BIN(s, b, a, >=);
	CHECK_BIN(s, a, b, !=);
	CHECK_EQ(s, a / b, "some/other/foo/bar");

	if (!s.str().empty())
		return Outcome(Failed { tiny_string(s.str()) });
	return Outcome(OutcomeType::Passed);
}

// TODO: Add `iostream` support to `Path`.
TEST_CASE_DECL(Path, iostream)
{
	return Outcome(OutcomeType::Ignored);
}
