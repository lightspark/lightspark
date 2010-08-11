# - Find libavcodec (FFMPEG)
# Find the native FFMPEG headers and libraries.
# Only grabs what Lightspark needs currently
#
#  FFMPEG_INCLUDE_DIRS - where to find avcodec.h
#  FFMPEG_LIBRARIES    - List of libraries when using ffmpeg
#  FFMPEG_FOUND        - True if ffmpeg found.

# Look for the header file.
FIND_PATH(FFMPEG_INCLUDE_DIR NAMES libavcodec/avcodec.h)
MARK_AS_ADVANCED(FFMPEG_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(FFMPEG_AVCODEC_LIBRARY NAMES 
    avcodec
)

FIND_LIBRARY(FFMPEG_AVUTIL_LIBRARY NAMES
    avutil
)

SET(FFMPEG_LIBRARY ${FFMPEG_AVCODEC_LIBRARY} ${FFMPEG_AVUTIL_LIBRARY})
MARK_AS_ADVANCED(FFMPEG_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set FFMPEG_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FFMPEG DEFAULT_MSG FFMPEG_LIBRARY FFMPEG_INCLUDE_DIR)

IF(FFMPEG_FOUND)
  SET(FFMPEG_LIBRARIES ${FFMPEG_LIBRARY})
  SET(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
ENDIF(FFMPEG_FOUND)
