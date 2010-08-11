# - Find PTHREADS
# Find the native PTHREADS headers and libraries.
#
#  PTHREADS_INCLUDE_DIRS - where to find pthread.h, etc.
#  PTHREADS_LIBRARIES    - List of libraries when using pthreads
#  PTHREADS_FOUND        - True if pthreads found.

#  Is this all we need on Linux?
# FIND_LIBRARY(lib_pthread pthread REQUIRED)

# Look for the header file.
FIND_PATH(PTHREADS_INCLUDE_DIR NAMES pthread.h)
MARK_AS_ADVANCED(PTHREADS_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(PTHREADS_LIBRARY NAMES 
    pthreadVC2d
)
MARK_AS_ADVANCED(PTHREADS_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set PTHREADS_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PTHREADS DEFAULT_MSG PTHREADS_LIBRARY PTHREADS_INCLUDE_DIR)

IF(PTHREADS_FOUND)
  SET(Threads_LIBRARIES ${PTHREADS_LIBRARY})
  SET(Threads_INCLUDE_DIR ${PTHREADS_INCLUDE_DIR})
ENDIF(PTHREADS_FOUND)
