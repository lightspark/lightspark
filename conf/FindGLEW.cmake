# - Find GLEW
# Find the native GLEW headers and libraries.
#
#  GLEW_INCLUDE_DIRS - where to find glew.h, etc.
#  GLEW_LIBRARIES    - List of libraries when using glews
#  GLEW_FOUND        - True if glews found.

# Look for the header file.
FIND_PATH(GLEW_INCLUDE_DIR NAMES GL/glew.h)
MARK_AS_ADVANCED(GLEW_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(GLEW_LIBRARY NAMES 
    GLEW
    glew32
)
MARK_AS_ADVANCED(GLEW_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set GLEW_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLEW DEFAULT_MSG GLEW_LIBRARY GLEW_INCLUDE_DIR)

IF(GLEW_FOUND)
  SET(GLEW_LIBRARIES ${GLEW_LIBRARY})
  SET(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
ENDIF(GLEW_FOUND)
