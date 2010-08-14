# - Find stdint
# Find the win32 stdint headers
#
#  STDINT_INCLUDE_DIRS - where to find stdint
#  STDINT_FOUND        - True if glews found.

# Look for the header file.
FIND_PATH(STDINT_INCLUDE_DIR NAMES stdint.h)
MARK_AS_ADVANCED(STDINT_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED arguments and set STDINT_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(STDINT DEFAULT_MSG STDINT_INCLUDE_DIR)

IF(STDINT_FOUND)
  SET(STDINT_INCLUDE_DIRS ${STDINT_INCLUDE_DIR})
ENDIF(STDINT_FOUND)
