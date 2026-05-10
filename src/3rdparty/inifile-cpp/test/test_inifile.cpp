/*
 * test_inifile.cpp
 *
 * Created on: 26 Dec 2015
 *     Author: Fabian Meyer
 *    License: MIT
 */

#include "inicpp.h"
#include <catch2/catch.hpp>
#include <cstring>
#include <sstream>

TEST_CASE("parse ini file", "IniFile")
{
    std::istringstream ss(("[Foo]\nbar=hello world\n[Test]"));
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 2);
    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "hello world");
    REQUIRE(inif["Test"].size() == 0);
}

TEST_CASE("parse empty file", "IniFile")
{
    std::istringstream ss("");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 0);
}

TEST_CASE("parse comment only file", "IniFile")
{
    std::istringstream ss("# this is a comment");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 0);
}

TEST_CASE("parse empty section", "IniFile")
{
    std::istringstream ss("[Foo]");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 0);
}

TEST_CASE("parse empty field", "IniFile")
{
    std::istringstream ss("[Foo]\nbar=");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "");
}

TEST_CASE("parse section with duplicate field", "IniFile")
{
    std::istringstream ss("[Foo]\nbar=hello\nbar=world");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "world");
}

TEST_CASE("parse section with duplicate field and overwriteDuplicateFields_ set to true", "IniFile")
{
    ini::IniFile inif;
    inif.allowOverwriteDuplicateFields(true);
    inif.decode("[Foo]\nbar=hello\nbar=world");

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "world");
}

TEST_CASE("parse field as bool", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=true\nbar2=false\nbar3=tRuE");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 3);
    REQUIRE(inif["Foo"]["bar1"].as<bool>());
    REQUIRE_FALSE(inif["Foo"]["bar2"].as<bool>());
    REQUIRE(inif["Foo"]["bar3"].as<bool>());
}

TEST_CASE("parse field as char", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=c\nbar2=q");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(inif["Foo"]["bar1"].as<char>() == 'c');
    REQUIRE(inif["Foo"]["bar2"].as<char>() == 'q');
}

TEST_CASE("parse field as unsigned char", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=c\nbar2=q");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(inif["Foo"]["bar1"].as<unsigned char>() == 'c');
    REQUIRE(inif["Foo"]["bar2"].as<unsigned char>() == 'q');
}

TEST_CASE("parse field as short", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=1\nbar2=-2");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(inif["Foo"]["bar1"].as<short>() == 1);
    REQUIRE(inif["Foo"]["bar2"].as<short>() == -2);
}

TEST_CASE("parse field as unsigned short", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=1\nbar2=13");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(inif["Foo"]["bar1"].as<unsigned short>() == 1);
    REQUIRE(inif["Foo"]["bar2"].as<unsigned short>() == 13);
}

TEST_CASE("parse field as int", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=1\nbar2=-2");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(inif["Foo"]["bar1"].as<int>() == 1);
    REQUIRE(inif["Foo"]["bar2"].as<int>() == -2);
}

TEST_CASE("parse field as unsigned int", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=1\nbar2=13");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(inif["Foo"]["bar1"].as<unsigned int>() == 1);
    REQUIRE(inif["Foo"]["bar2"].as<unsigned int>() == 13);
}

TEST_CASE("parse field as long", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=1\nbar2=-2");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(inif["Foo"]["bar1"].as<long>() == 1);
    REQUIRE(inif["Foo"]["bar2"].as<long>() == -2);
}

TEST_CASE("parse field as unsigned long", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=1\nbar2=13");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(inif["Foo"]["bar1"].as<unsigned long>() == 1);
    REQUIRE(inif["Foo"]["bar2"].as<unsigned long>() == 13);
}

TEST_CASE("parse field as double", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=1.2\nbar2=1\nbar3=-2.4");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 3);
    REQUIRE(inif["Foo"]["bar1"].as<double>() == Approx(1.2).margin(1e-3));
    REQUIRE(inif["Foo"]["bar2"].as<double>() == Approx(1.0).margin(1e-3));
    REQUIRE(inif["Foo"]["bar3"].as<double>() == Approx(-2.4).margin(1e-3));
}

TEST_CASE("parse field as float", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=1.2\nbar2=1\nbar3=-2.4");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 3);
    REQUIRE(inif["Foo"]["bar1"].as<float>() == Approx(1.2f).margin(1e-3f));
    REQUIRE(inif["Foo"]["bar2"].as<float>() == Approx(1.0f).margin(1e-3f));
    REQUIRE(inif["Foo"]["bar3"].as<float>() == Approx(-2.4f).margin(1e-3f));
}

TEST_CASE("parse field as std::string", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=hello\nbar2=world");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(inif["Foo"]["bar1"].as<std::string>() == "hello");
    REQUIRE(inif["Foo"]["bar2"].as<std::string>() == "world");
}

TEST_CASE("parse field as const char*", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=hello\nbar2=world");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(std::strcmp(inif["Foo"]["bar1"].as<const char *>(), "hello") == 0);
    REQUIRE(std::strcmp(inif["Foo"]["bar2"].as<const char *>(), "world") == 0);
}

#ifdef __cpp_lib_string_view
TEST_CASE("parse field as std::string_view", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1=hello\nbar2=world");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 2);
    REQUIRE(inif["Foo"]["bar1"].as<std::string_view>() == "hello");
    REQUIRE(inif["Foo"]["bar2"].as<std::string_view>() == "world");
}
#endif

TEST_CASE("parse field with custom field sep", "IniFile")
{
    std::istringstream ss("[Foo]\nbar1:true\nbar2:false\nbar3:tRuE");
    ini::IniFile inif;

    inif.setFieldSep(':');
    inif.decode(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 3);
    REQUIRE(inif["Foo"]["bar1"].as<bool>());
    REQUIRE_FALSE(inif["Foo"]["bar2"].as<bool>());
    REQUIRE(inif["Foo"]["bar3"].as<bool>());
}

TEST_CASE("parse with comment", "IniFile")
{
    std::istringstream ss("[Foo]\n# this is a test\nbar=bla");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "bla");
}

TEST_CASE("parse with custom comment char prefix", "IniFile")
{
    std::istringstream ss("[Foo]\n$ this is a test\nbar=bla");
    ini::IniFile inif;

    inif.setFieldSep('=');
    inif.setCommentChar('$');
    inif.decode(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "bla");
}

TEST_CASE("parse with multi char comment prefix", "IniFile")
{
    std::istringstream ss("[Foo]\nREM this is a test\nbar=bla");
    ini::IniFile inif(ss, '=', {"REM"});

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "bla");
}

TEST_CASE("parse with multiple multi char comment prefixes", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "REM this is a comment\n"
                          "#Also a comment\n"
                          "//Even this is a comment\n"
                          "bar=bla");
    ini::IniFile inif(ss, '=', {"REM", "#", "//"});

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "bla");
}

TEST_CASE("comment prefixes can be set after construction", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "REM this is a comment\n"
                          "#Also a comment\n"
                          "//Even this is a comment\n"
                          "bar=bla");
    ini::IniFile inif;
    inif.setCommentPrefixes({"REM", "#", "//"});
    inif.decode(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "bla");
}

TEST_CASE("comments are allowed after escaped comments", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "hello=world \\## this is a comment\n"
                          "more=of this \\# \\#\n");
    ini::IniFile inif(ss);

    REQUIRE(inif["Foo"]["hello"].as<std::string>() == "world #");
    REQUIRE(inif["Foo"]["more"].as<std::string>() == "of this # #");
}

TEST_CASE("escape char right before a comment prefix escapes all the comment prefix", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "weird1=note \\### this is not a comment\n"
                          "weird2=but \\#### this is a comment");
    ini::IniFile inif(ss, '=', {"##"});

    REQUIRE(inif["Foo"]["weird1"].as<std::string>() == "note ### this is not a comment");
    REQUIRE(inif["Foo"]["weird2"].as<std::string>() == "but ##");
}

TEST_CASE("escape comment when writing", "IniFile")
{
    ini::IniFile inif('=', {"#"});

    inif["Fo#o"] = ini::IniSection();
    inif["Fo#o"]["he#llo"] = "world";
    inif["Fo#o"]["world"] = "he#llo";

    std::string str = inif.encode();

    REQUIRE(str == "[Fo\\#o]\n"
                   "he\\#llo=world\n"
                   "world=he\\#llo\n\n");
}

TEST_CASE("decode what we encoded", "IniFile")
{
    std::string content = "[Fo\\#o]\n"
                          "he\\REMllo=worl\\REMd\n"
                          "world=he\\//llo\n\n";

    ini::IniFile inif('=', {"#", "REM", "//"});

    // decode the string
    inif.decode(content);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif.find("Fo#o") != inif.end());
    REQUIRE(inif["Fo#o"].size() == 2);
    REQUIRE(inif["Fo#o"].find("heREMllo") != inif["Fo#o"].end());
    REQUIRE(inif["Fo#o"].find("world") != inif["Fo#o"].end());
    REQUIRE(inif["Fo#o"]["heREMllo"].as<std::string>() == "worlREMd");
    REQUIRE(inif["Fo#o"]["world"].as<std::string>() == "he//llo");

    auto actual = inif.encode();

    REQUIRE(content == actual);

    inif.decode(actual);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif.find("Fo#o") != inif.end());
    REQUIRE(inif["Fo#o"].size() == 2);
    REQUIRE(inif["Fo#o"].find("heREMllo") != inif["Fo#o"].end());
    REQUIRE(inif["Fo#o"].find("world") != inif["Fo#o"].end());
    REQUIRE(inif["Fo#o"]["heREMllo"].as<std::string>() == "worlREMd");
    REQUIRE(inif["Fo#o"]["world"].as<std::string>() == "he//llo");

    auto actual2 = inif.encode();

    REQUIRE(content == actual2);
}

TEST_CASE("save with bool fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = true;
    inif["Foo"]["bar2"] = false;

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=true\nbar2=false\n\n");
}

TEST_CASE("save with char fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<char>('c');
    inif["Foo"]["bar2"] = static_cast<char>('q');

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=c\nbar2=q\n\n");
}

TEST_CASE("save with unsigned char fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<unsigned char>('c');
    inif["Foo"]["bar2"] = static_cast<unsigned char>('q');

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=c\nbar2=q\n\n");
}

TEST_CASE("save with short fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<short>(1);
    inif["Foo"]["bar2"] = static_cast<short>(-2);

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=1\nbar2=-2\n\n");
}

TEST_CASE("save with unsigned short fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<unsigned short>(1);
    inif["Foo"]["bar2"] = static_cast<unsigned short>(13);

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=1\nbar2=13\n\n");
}

TEST_CASE("save with int fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<int>(1);
    inif["Foo"]["bar2"] = static_cast<int>(-2);

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=1\nbar2=-2\n\n");
}

TEST_CASE("save with unsigned int fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<unsigned int>(1);
    inif["Foo"]["bar2"] = static_cast<unsigned int>(13);

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=1\nbar2=13\n\n");
}

TEST_CASE("save with long fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<long>(1);
    inif["Foo"]["bar2"] = static_cast<long>(-2);

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=1\nbar2=-2\n\n");
}

TEST_CASE("save with unsigned long fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<unsigned long>(1);
    inif["Foo"]["bar2"] = static_cast<unsigned long>(13);

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=1\nbar2=13\n\n");
}

TEST_CASE("save with double fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<double>(1.2);
    inif["Foo"]["bar2"] = static_cast<double>(-2.4);

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=1.2\nbar2=-2.4\n\n");
}

TEST_CASE("save with float fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<float>(1.2f);
    inif["Foo"]["bar2"] = static_cast<float>(-2.4f);

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=1.2\nbar2=-2.4\n\n");
}

TEST_CASE("save with std::string fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<std::string>("hello");
    inif["Foo"]["bar2"] = static_cast<std::string>("world");

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=hello\nbar2=world\n\n");
}

TEST_CASE("save with const char* fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = static_cast<const char *>("hello");
    inif["Foo"]["bar2"] = static_cast<const char *>("world");

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=hello\nbar2=world\n\n");
}

TEST_CASE("save with char* fields", "IniFile")
{
    ini::IniFile inif;
    char bar1[6] = "hello";
    char bar2[6] = "world";
    inif["Foo"]["bar1"] = static_cast<char *>(bar1);
    inif["Foo"]["bar2"] = static_cast<char *>(bar2);

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=hello\nbar2=world\n\n");
}

TEST_CASE("save with string literal fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = "hello";
    inif["Foo"]["bar2"] = "world";

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=hello\nbar2=world\n\n");
}

#ifdef __cpp_lib_string_view
TEST_CASE("save with std::string_view fields", "IniFile")
{
    ini::IniFile inif;
    inif["Foo"]["bar1"] = std::string_view("hello");
    inif["Foo"]["bar2"] = std::string_view("world");

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1=hello\nbar2=world\n\n");
}
#endif

TEST_CASE("save with custom field sep", "IniFile")
{
    ini::IniFile inif(':', '#');
    inif["Foo"]["bar1"] = true;
    inif["Foo"]["bar2"] = false;

    std::string result = inif.encode();
    REQUIRE(result == "[Foo]\nbar1:true\nbar2:false\n\n");
}

TEST_CASE("inline comments in sections are discarded", "IniFile")
{
    std::istringstream ss("[Foo] # This is an inline comment\nbar=Hello world!");
    ini::IniFile inif(ss);

    REQUIRE(inif.find("Foo") != inif.end());
}

TEST_CASE("inline comments in fields are discarded", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "bar=Hello #world!");
    ini::IniFile inif(ss);

    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "Hello");
}

TEST_CASE("inline comments can be escaped", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "bar=Hello \\#world!");
    ini::IniFile inif(ss);

    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "Hello #world!");
}

TEST_CASE("escape characters are kept if not before a comment prefix", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "bar=Hello \\world!");
    ini::IniFile inif(ss);

    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "Hello \\world!");
}

TEST_CASE("multi-line values are read correctly with space indents", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "bar=Hello\n"
                          "    world!");
    ini::IniFile inif;
    inif.setMultiLineValues(true);
    inif.decode(ss);

    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "Hello\nworld!");
}

TEST_CASE("multi-line values are read correctly with tab indents", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "bar=Hello\n"
                          "\tworld!");
    ini::IniFile inif;
    inif.setMultiLineValues(true);
    inif.decode(ss);

    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "Hello\nworld!");
}

TEST_CASE("multi-line values discard end-of-line comments", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "bar=Hello ; everyone\n"
                          "    world! ; comment");
    ini::IniFile inif;
    inif.setMultiLineValues(true);
    inif.decode(ss);

    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "Hello\nworld!");
}

TEST_CASE("multi-line values discard interspersed comment lines", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "bar=Hello\n"
                          "; everyone\n"
                          "    world!");
    ini::IniFile inif;
    inif.setMultiLineValues(true);
    inif.decode(ss);

    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "Hello\nworld!");
}

TEST_CASE("multi-line values should not be parsed when disabled", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "    bar=Hello\n"
                          "    baz=world!");
    ini::IniFile inif;
    inif.setMultiLineValues(false);
    inif.decode(ss);

    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "Hello");
    REQUIRE(inif["Foo"]["baz"].as<std::string>() == "world!");
}

TEST_CASE("multi-line values should be parsed when enabled, even when the continuation contains =", "IniFile")
{
    std::istringstream ss("[Foo]\n"
                          "    bar=Hello\n"
                          "    baz=world!");
    ini::IniFile inif;
    inif.setMultiLineValues(true);
    inif.decode(ss);

    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "Hello\nbaz=world!");
    REQUIRE(inif["Foo"]["baz"].as<std::string>() == "");
}

TEST_CASE("when multi-line values are enabled, write newlines as multi-line value continuations", "IniFile")
{
    ini::IniFile inif;
    inif.setMultiLineValues(true);

    inif["Foo"] = ini::IniSection();
    inif["Foo"]["bar"] = "Hello\nworld!";

    std::string str = inif.encode();

    REQUIRE(str == "[Foo]\n"
                   "bar=Hello\n"
                   "\tworld!\n\n");
}

TEST_CASE("stringInsensitiveLess operator() returns true if and only if first parameter is less than the second "
          "ignoring sensitivity",
    "StringInsensitiveLessFunctor")
{
    ini::StringInsensitiveLess cc;

    REQUIRE(cc("a", "b"));
    REQUIRE(cc("a", "B"));
    REQUIRE(cc("aaa", "aaB"));
}

TEST_CASE(
    "stringInsensitiveLess operator() returns false when words differs only in case", "StringInsensitiveLessFunctor")
{
    ini::StringInsensitiveLess cc;

    REQUIRE(cc("AA", "aa") == false);
}

TEST_CASE("stringInsensitiveLess operator() has a case insensitive strict weak ordering policy",
    "StringInsensitiveLessFunctor")
{
    ini::StringInsensitiveLess cc;

    REQUIRE(cc("B", "a") == false);
}

TEST_CASE("default inifile parser is case sensitive", "IniFile")
{
    std::istringstream ss("[FOO]\nbar=bla");
    ini::IniFile inif(ss);

    REQUIRE(inif.find("foo") == inif.end());
    REQUIRE(inif["FOO"].find("BAR") == inif["FOO"].end());
}

TEST_CASE("case insensitive inifile ignores case of section", "IniFile")
{
    std::istringstream ss("[FOO]\nbar=bla");
    ini::IniFileCaseInsensitive inif(ss);

    REQUIRE(inif.find("foo") != inif.end());
    REQUIRE(inif.find("FOO") != inif.end());
}

TEST_CASE("case insensitive inifile ignores case of field", "IniFile")
{
    std::istringstream ss("[FOO]\nbar=bla");
    ini::IniFileCaseInsensitive inif(ss);

    REQUIRE(inif["FOO"].find("BAR") != inif["FOO"].end());
}

TEST_CASE(".as<>() works with IniFileCaseInsensitive", "IniFile")
{
    std::istringstream ss("[FOO]\nbar=bla");
    ini::IniFileCaseInsensitive inif(ss);

    REQUIRE(inif["FOO"]["bar"].as<std::string>() == "bla");
}

TEST_CASE("trim() works with empty strings", "TrimFunction")
{
    std::string example1 = "";
    std::string example2 = "  \t\n  ";

    ini::trim(example1);
    ini::trim(example2);

    REQUIRE(example1.size() == 0);
    REQUIRE(example2.size() == 0);
}

TEST_CASE("trim() works with already trimmed strings", "TrimFunction")
{
    std::string example1 = "example_text";
    std::string example2 = "example  \t\n  text";

    ini::trim(example1);
    ini::trim(example2);

    REQUIRE(example1 == "example_text");
    REQUIRE(example2 == "example  \t\n  text");
}

TEST_CASE("trim() works with untrimmed strings", "TrimFunction")
{
    std::string example1 = "example text      ";
    std::string example2 = "      example text";
    std::string example3 = "      example text      ";
    std::string example4 = "  \t\n  example  \t\n  text  \t\n  ";

    ini::trim(example1);
    ini::trim(example2);
    ini::trim(example3);
    ini::trim(example4);

    REQUIRE(example1 == "example text");
    REQUIRE(example2 == "example text");
    REQUIRE(example3 == "example text");
    REQUIRE(example4 == "example  \t\n  text");
}

/***************************************************
 *                Failing Tests
 ***************************************************/

TEST_CASE("fail to load unclosed section", "IniFile")
{
    ini::IniFile inif;
    REQUIRE_THROWS(inif.decode("[Foo\nbar=bla"));
}

TEST_CASE("fail to load field without equal", "IniFile")
{
    ini::IniFile inif;
    REQUIRE_THROWS(inif.decode("[Foo]\nbar"));
}

TEST_CASE("fail to parse a multi-line field without indentation (when enabled)", "IniFile")
{
    ini::IniFile inif;
    inif.setMultiLineValues(true);
    REQUIRE_THROWS(inif.decode("[Foo]\nbar=Hello\nworld!"));
}

TEST_CASE("fail to parse a multi-line field without indentation (when disabled)", "IniFile")
{
    ini::IniFile inif;
    inif.setMultiLineValues(false);
    REQUIRE_THROWS(inif.decode("[Foo]\nbar=Hello\nworld!"));
}

TEST_CASE("fail to continue multi-line field without start (when enabled)", "IniFile")
{
    ini::IniFile inif;
    inif.setMultiLineValues(true);
    REQUIRE_THROWS(inif.decode("[Foo]\n    world!\nbar=Hello"));
}

TEST_CASE("fail to continue multi-line field without start (when disabled)", "IniFile")
{
    ini::IniFile inif;
    inif.setMultiLineValues(false);
    REQUIRE_THROWS(inif.decode("[Foo]\n    world!\nbar=Hello"));
}

TEST_CASE("fail to parse as bool", "IniFile")
{
    std::istringstream ss("[Foo]\nbar=bla");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE_THROWS(inif["Foo"]["bar"].as<bool>());
}

TEST_CASE("fail to parse as int", "IniFile")
{
    std::istringstream ss("[Foo]\nbar=bla");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE_THROWS(inif["Foo"]["bar"].as<int>());
}

TEST_CASE("fail to parse as double", "IniFile")
{
    std::istringstream ss("[Foo]\nbar=bla");
    ini::IniFile inif(ss);

    REQUIRE(inif.size() == 1);
    REQUIRE(inif["Foo"].size() == 1);
    REQUIRE_THROWS(inif["Foo"]["bar"].as<double>());
}

TEST_CASE("fail to parse field without section", "IniFile")
{
    ini::IniFile inif;
    REQUIRE_THROWS(inif.decode("bar=bla"));
}

TEST_CASE("spaces are not taken into account in field names", "IniFile")
{
    std::istringstream ss(("[Foo]\n  \t  bar  \t  =hello world"));
    ini::IniFile inif(ss);

    REQUIRE(inif["Foo"].find("bar") != inif["Foo"].end());
    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "hello world");
}

TEST_CASE("spaces are not taken into account in field values", "IniFile")
{
    std::istringstream ss(("[Foo]\nbar=  \t  hello world  \t  "));
    ini::IniFile inif(ss);

    REQUIRE(inif["Foo"]["bar"].as<std::string>() == "hello world");
}

TEST_CASE("spaces are not taken into account in sections", "IniFile")
{
    std::istringstream ss("  \t  [Foo]  \t  \nbar=bla");
    ini::IniFile inif(ss);

    REQUIRE(inif.find("Foo") != inif.end());
}

TEST_CASE("parse section with duplicate field and overwriteDuplicateFields_ set to false", "IniFile")
{
    ini::IniFile inif;
    inif.allowOverwriteDuplicateFields(false);
    REQUIRE_THROWS(inif.decode("[Foo]\nbar=hello\nbar=world"));
}
