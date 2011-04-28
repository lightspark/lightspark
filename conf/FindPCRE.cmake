# - Find PCRE
# Find the native PCRE headers and libraries.
#
#  PCRE_INCLUDE_DIRS - where to find pcre.h, etc.
#  PCRE_LIBRARIES    - List of libraries when using pcrecpp
#  PCRE_FOUND        - True if pcrecpp found.

# Look for the header file.
FIND_PATH(PCRE_INCLUDE_DIR NAMES pcre.h)
MARK_AS_ADVANCED(PCRE_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(PCRE_LIBRARY NAMES
    pcre
    pcred
    PATH_SUFFIXES lib64 lib x86_64-linux-gnu i386-linux-gnu
)
MARK_AS_ADVANCED(PCRE_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set PCRECPP_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCRE DEFAULT_MSG PCRE_LIBRARY PCRE_INCLUDE_DIR PCRE_LIBRARY)

IF(PCRE_FOUND)
  SET(PCRE_LIBRARIES ${PCRE_LIBRARY})
  SET(PCRE_INCLUDE_DIRS ${PCRE_INCLUDE_DIR})
ENDIF(PCRE_FOUND)
