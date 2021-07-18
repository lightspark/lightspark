/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */


/* On Unix-like systems config.h.in is converted by "configure" into config.h.
Some other environments also support the use of "configure". PCRE is written in
Standard C, but there are a few non-standard things it can cope with, allowing
it to run on SunOS4 and other "close to standard" systems.

If you are going to build PCRE "by hand" on a system without "configure" you
should copy the distributed config.h.generic to config.h, and then set up the
macros the way you need them. Alternatively, you can avoid editing by using -D
on the compiler command line to set the macro values.

PCRE uses memmove() if HAVE_MEMMOVE is set to 1; otherwise it uses bcopy() if
HAVE_BCOPY is set to 1. If your system has neither bcopy() nor memmove(), set
them both to 0; an emulation function will be used. */

/* If you are compiling for a system that uses EBCDIC instead of ASCII
   character codes, define this macro as 1. On systems that can use
   "configure", this can be done via --enable-ebcdic. */
/* #undef EBCDIC */

/* Define to 1 if you have the `bcopy' function. */
#define HAVE_BCOPY 1

/* Define to 1 if you have the <bits/type_traits.h> header file. */
#define HAVE_BITS_TYPE_TRAITS_H 1

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if the system has the type `long long'. */
#ifndef HAVE_LONG_LONG
#define HAVE_LONG_LONG 1
#endif

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <string> header file. */
#define HAVE_STRING 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strtoll' function. */
#define HAVE_STRTOLL 1

/* Define to 1 if you have the `strtoq' function. */
/* #undef HAVE_STRTOQ */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <type_traits.h> header file. */
/* #undef HAVE_TYPE_TRAITS_H */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if the system has the type `unsigned long long'. */
#define HAVE_UNSIGNED_LONG_LONG 1

/* Define to 1 if you have the <windows.h> header file. */
#define HAVE_WINDOWS_H 1

/* cn: added constants for carriage return, unicode line seperator, and unicode paragraph seperator,
       all of which should match the '$' metacharater.
*/
//#define CARRAIGE_RETURN '\r'					// <CR>
//#define UNICODE_LINE_SEPERATOR 0x2028			// <LS>
//#define UNICODE_PARAGRAPH_SEPERATOR 0x2029		// <PS>

// cn: also added unicode whitespace characters.  
#define UNICODE_NOBREAK_SPACE 0x00A0			// - no-break space
#define UNICODE_ZEROWIDTH_SPACE 0x200B			// - zero width space
#define UNICODE_WORDJOINER_SPACE 0x2060			// - word joiner
#define UNICODE_IDEOGRAPHIC_SPACE 0x3000		// - ideographic space
#define UNICODE_ZEROWIDTH_NOBREAK_SPACE 0xFEFF	// - zero width no-break space

//inline bool isECMALineTerminator(unsigned short x) { return (((x) == NEWLINE) || ((x) == CARRAIGE_RETURN) || ((x) == UNICODE_LINE_SEPERATOR) || ((x) == UNICODE_PARAGRAPH_SEPERATOR) ); }
bool isUnicodeWhiteSpace(unsigned short x);

/* The value of LINK_SIZE determines the number of bytes used to store links
   as offsets within the compiled regex. The default is 2, which allows for
   compiled patterns up to 64K long. This covers the vast majority of cases.
   However, PCRE can also be compiled to use 3 or 4 bytes instead. This allows
   for longer patterns in extreme cases. On systems that support it,
   "configure" can be used to override this default. */
#define LINK_SIZE 2

/* The value of MATCH_LIMIT determines the default number of times the
   internal match() function can be called during a single execution of
   pcre_exec(). There is a runtime interface for setting a different limit.
   The limit exists in order to catch runaway regular expressions that take
   for ever to determine that they do not match. The default is set very large
   so that it does not accidentally catch legitimate cases. On systems that
   support it, "configure" can be used to override this default default. */
#define MATCH_LIMIT 10000000

/* The above limit applies to all calls of match(), whether or not they
   increase the recursion depth. In some environments it is desirable to limit
   the depth of recursive calls of match() more strictly, in order to restrict
   the maximum amount of stack (or heap, if NO_RECURSE is defined) that is
   used. The value of MATCH_LIMIT_RECURSION applies only to recursive calls of
   match(). To have any useful effect, it must be less than the value of
   MATCH_LIMIT. The default is to use the same value as MATCH_LIMIT. There is
   a runtime method for setting a different limit. On systems that support it,
   "configure" can be used to override the default. */
#define MATCH_LIMIT_RECURSION MATCH_LIMIT

/* This limit is parameterized just in case anybody ever wants to change it.
   Care must be taken if it is increased, because it guards against integer
   overflow caused by enormously large patterns. */
#define MAX_NAME_COUNT 10000

/* This limit is parameterized just in case anybody ever wants to change it.
   Care must be taken if it is increased, because it guards against integer
   overflow caused by enormously large patterns. */
#define MAX_NAME_SIZE 32

/* The value of NEWLINE determines the newline character sequence. On
   Unix-like systems, "configure" can be used to override the default, which
   is 10. The possible values are 10 (LF), 13 (CR), 3338 (CRLF), -1 (ANY), or
   -2 (ANYCRLF). */
#define NEWLINE -1

/* PCRE uses recursive function calls to handle backtracking while matching.
   This can sometimes be a problem on systems that have stacks of limited
   size. Define NO_RECURSE to get a version that doesn't use recursion in the
   match() function; instead it creates its own stack by steam using
   pcre_recurse_malloc() to obtain memory from the heap. For more detail, see
   the comments and other stuff just above the match() function. On systems
   that support it, "configure" can be used to set this in the Makefile (use
   --disable-stack-for-recursion). */
#define NO_RECURSE 

/* Name of package */
#ifndef PACKAGE
#define PACKAGE "pcre"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "PCRE"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "PCRE 7.3"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "pcre"

/* Define to the version of this package. */
#define PACKAGE_VERSION "7.3"
#endif // PACKAGE

/* If you are compiling for a system other than a Unix-like system or
   Win32, and it needs some magic to be inserted before the definition
   of a function that is exported by the library, define this macro to
   contain the relevant magic. If you do not define this macro, it
   defaults to "extern" for a C compiler and "extern C" for a C++
   compiler on non-Win32 systems. This macro apears at the start of
   every exported function that is part of the external API. It does
   not appear on functions that are "external" in the C sense, but
   which are internal to the library. */
/* #undef PCRE_EXP_DEFN */

/* Define if linking statically (TODO: make nice with Libtool) */
/* #undef PCRE_STATIC */

/* When calling PCRE via the POSIX interface, additional working storage is
   required for holding the pointers to capturing substrings because PCRE
   requires three integers per substring, whereas the POSIX interface provides
   only two. If the number of expected substrings is small, the wrapper
   function uses space on the stack, because this is faster than using
   malloc() for each call. The threshold above which the stack is no longer
   used is defined by POSIX_MALLOC_THRESHOLD. On systems that support it,
   "configure" can be used to override this default. */
#define POSIX_MALLOC_THRESHOLD 10

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to enable support for Unicode properties */
/* #undef SUPPORT_UCP */

/* Define to enable support for the UTF-8 Unicode encoding. */
#define SUPPORT_UTF8 

/* Version number of package */
#ifndef VERSION
#define VERSION "7.3"
#endif // VERSION

//#define PCRE_DEBUG

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

