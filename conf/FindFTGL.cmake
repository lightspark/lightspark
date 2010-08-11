# - Find FTGL
# Find the native FTGL headers and libraries.
#
#  FTGL_INCLUDE_DIRS - where to find pcre.h, etc.
#  FTGL_LIBRARIES    - List of libraries when using ftgl
#  FTGL_FOUND        - True if ftgl found.

# Look for the header file.
FIND_PATH(FTGL_INCLUDE_DIR NAMES FTGL/ftgl.h)
MARK_AS_ADVANCED(FTGL_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(FTGL_LIBRARY NAMES 
    ftgl
    ftgl_d
)
MARK_AS_ADVANCED(FTGL_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set FTGL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FTGL DEFAULT_MSG FTGL_LIBRARY FTGL_INCLUDE_DIR)

IF(FTGL_FOUND)
  SET(FTGL_LIBRARIES ${FTGL_LIBRARY})
  SET(FTGL_INCLUDE_DIRS ${FTGL_INCLUDE_DIR})
ENDIF(FTGL_FOUND)
