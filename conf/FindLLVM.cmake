# Detect LLVM and set various variable to link against the different component of LLVM
#
# NOTE: This is a modified version of the module originally found in the OpenGTL project
# at www.opengtl.org
#
# LLVM_BIN_DIR : directory with LLVM binaries
# LLVM_LIB_DIR : directory with LLVM library
# LLVM_INCLUDE_DIR : directory with LLVM include
#
# LLVM_COMPILE_FLAGS : compile flags needed to build a program using LLVM headers
# LLVM_LDFLAGS : ldflags needed to link
# LLVM_LIBS_CORE : ldflags needed to link against a LLVM core library
# LLVM_LIBS_JIT : ldflags needed to link against a LLVM JIT
# LLVM_LIBS_JIT_OBJECTS : objects you need to add to your source when using LLVM JIT

if(WIN32)
  find_path(LLVM_INCLUDE_DIR NAMES llvm/LLVMContext.h)
  message(STATUS "Found LLVM include directory: ${LLVM_INCLUDE_DIR}")
  find_library(LLVM_SOMELIB NAMES LLVMCore)
  GET_FILENAME_COMPONENT(LLVM_LIB_DIR ${LLVM_SOMELIB} PATH CACHE)
  message(STATUS "Found LLVM lib directory: ${LLVM_LIB_DIR}")

  #Starting from 3.0, that file exists
  find_path(LLVM_3_0 NAMES llvm/Support/TargetSelect.h)
  if(LLVM_3_0)
    set(LLVM_STRING_VERSION "3.0")
    set(LLVM_LIBS_CORE LLVMLinker LLVMArchive LLVMBitWriter LLVMBitReader LLVMInstrumentation LLVMipo LLVMInstCombine)
	set(LLVM_LIBS_JIT LLVMX86AsmParser LLVMX86AsmPrinter LLVMX86CodeGen LLVMX86Desc LLVMSelectionDAG LLVMAsmPrinter LLVMX86Utils LLVMX86Info LLVMJIT LLVMExecutionEngine LLVMCodeGen LLVMScalarOpts LLVMTransformUtils LLVMipa LLVMAnalysis LLVMTarget LLVMMC LLVMCore LLVMSupport)
  else()
    set(LLVM_STRING_VERSION "2.8")
    set(LLVM_LIBS_CORE LLVMLinker LLVMArchive LLVMBitWriter LLVMBitReader LLVMInstrumentation LLVMScalarOpts LLVMipo LLVMTransformUtils LLVMipa LLVMAnalysis LLVMTarget LLVMMC LLVMCore LLVMSupport LLVMSystem LLVMInstCombine)
    set(LLVM_LIBS_JIT LLVMX86AsmParser LLVMX86AsmPrinter LLVMX86CodeGen LLVMSelectionDAG LLVMAsmPrinter LLVMX86Info LLVMJIT LLVMExecutionEngine LLVMCodeGen LLVMScalarOpts LLVMTransformUtils LLVMipa LLVMAnalysis LLVMTarget LLVMMC LLVMCore LLVMSupport LLVMSystem)
  endif()
  message(STATUS "_Guessed_ LLVM version ${LLVM_STRING_VERSION}")
  set(LLVM_COMPILE_FLAGS "")
  set(LLVM_LDFLAGS "")
  set(LLVM_LIBS_JIT_OBJECTS "")
endif (WIN32)

if (LLVM_INCLUDE_DIR)
  set(LLVM_FOUND TRUE)
else (LLVM_INCLUDE_DIR)

  find_program(LLVM_CONFIG_EXECUTABLE
    NAMES llvm-config
    PATHS
    /opt/local/bin
    /opt/llvm/2.7/bin
    /opt/llvm/bin
    /usr/lib/llvm-2.7/bin
    /usr/lib/llvm-2.8/bin
    /usr/lib/llvm-2.9/bin
    /usr/lib/llvm-3.0/bin
    /usr/lib/llvm-3.1/bin
    /usr/lib/llvm-3.2/bin
    /usr/lib/llvm-3.3/bin
    /usr/lib/llvm-3.4/bin
    /usr/lib/llvm-3.5/bin
    /usr/lib/llvm-3.6/bin
    )

  find_program(LLVM_GCC_EXECUTABLE
    NAMES llvm-gcc llvmgcc
    PATHS
    /opt/local/bin
    /opt/llvm/2.7/bin
    /opt/llvm/bin
    /Developer/usr/bin
    /usr/lib/llvm-2.7/bin
    )

  find_program(LLVM_GXX_EXECUTABLE
    NAMES llvm-g++ llvmg++
    PATHS
    /opt/local/bin
    /opt/llvm/2.7/bin
    /opt/llvm/bin
    /Developer/usr/bin
    /usr/lib/llvm/llvm/gcc-4.2/bin
    /usr/lib/llvm-2.7/bin
    )

  if(LLVM_GCC_EXECUTABLE)
    MESSAGE(STATUS "LLVM llvm-gcc found at: ${LLVM_GCC_EXECUTABLE}")
    #CMAKE_FORCE_C_COMPILER(${LLVM_GCC_EXECUTABLE} GNU)
  endif(LLVM_GCC_EXECUTABLE)

  if(LLVM_GXX_EXECUTABLE)
    MESSAGE(STATUS "LLVM llvm-g++ found at: ${LLVM_GXX_EXECUTABLE}")
    #CMAKE_FORCE_CXX_COMPILER(${LLVM_GXX_EXECUTABLE} GNU)
  endif(LLVM_GXX_EXECUTABLE)
  
  if(LLVM_CONFIG_EXECUTABLE)
    MESSAGE(STATUS "LLVM llvm-config found at: ${LLVM_CONFIG_EXECUTABLE}")
  else(LLVM_CONFIG_EXECUTABLE)
    MESSAGE(FATAL_ERROR "Could NOT find LLVM executable")
  endif(LLVM_CONFIG_EXECUTABLE)

  MACRO(FIND_LLVM_LIBS LLVM_CONFIG_EXECUTABLE _libname_ LIB_VAR OBJECT_VAR)
    exec_program( ${LLVM_CONFIG_EXECUTABLE} ARGS --libs ${_libname_}  OUTPUT_VARIABLE ${LIB_VAR} )
    STRING(REGEX MATCHALL "[^ ]*[.]o[ $]"  ${OBJECT_VAR} ${${LIB_VAR}})
    SEPARATE_ARGUMENTS(${OBJECT_VAR})
    STRING(REGEX REPLACE "[^ ]*[.]o[ $]" ""  ${LIB_VAR} ${${LIB_VAR}})
    SEPARATE_ARGUMENTS(${LIB_VAR})
  ENDMACRO(FIND_LLVM_LIBS)
  
  
  # this function borrowed from PlPlot, Copyright (C) 2006  Alan W. Irwin
  function(TRANSFORM_VERSION numerical_result version)
    # internal_version ignores everything in version after any character that
    # is not 0-9 or ".".  This should take care of the case when there is
    # some non-numerical data in the patch version.
    #message(STATUS "DEBUG: version = ${version}")
    string(REGEX REPLACE "^([0-9.]+).*$" "\\1" internal_version ${version})
    
    # internal_version is normally a period-delimited triplet string of the form
    # "major.minor.patch", but patch and/or minor could be missing.
    # Transform internal_version into a numerical result that can be compared.
    string(REGEX REPLACE "^([0-9]*).+$" "\\1" major ${internal_version})
    string(REGEX REPLACE "^[0-9]*\\.([0-9]*).*$" "\\1" minor ${internal_version})
    #string(REGEX REPLACE "^[0-9]*\\.[0-9]*\\.([0-9]*)$" "\\1" patch ${internal_version})
    
    #if(NOT patch MATCHES "[0-9]+")
    #  set(patch 0)
    #endif(NOT patch MATCHES "[0-9]+")
    set(patch 0)
    
    if(NOT minor MATCHES "[0-9]+")
      set(minor 0)
    endif(NOT minor MATCHES "[0-9]+")
    
    if(NOT major MATCHES "[0-9]+")
      set(major 0)
    endif(NOT major MATCHES "[0-9]+")
    #message(STATUS "DEBUG: internal_version = ${internal_version}")
    #message(STATUS "DEBUG: major = ${major}")
    #message(STATUS "DEBUG: minor= ${minor}")
    #message(STATUS "DEBUG: patch = ${patch}")
    math(EXPR internal_numerical_result
      #"${major}*1000000 + ${minor}*1000 + ${patch}"
      "${major}*1000000 + ${minor}*1000"
      )
    #message(STATUS "DEBUG: ${numerical_result} = ${internal_numerical_result}")
    set(${numerical_result} ${internal_numerical_result} PARENT_SCOPE)
  endfunction(TRANSFORM_VERSION)
  
  
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --version OUTPUT_VARIABLE LLVM_STRING_VERSION )
  MESSAGE(STATUS "LLVM version: " ${LLVM_STRING_VERSION})
  transform_version(LLVM_VERSION ${LLVM_STRING_VERSION})

  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --bindir OUTPUT_VARIABLE LLVM_BIN_DIR )
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --libdir OUTPUT_VARIABLE LLVM_LIB_DIR )
  #MESSAGE(STATUS "LLVM lib dir: " ${LLVM_LIB_DIR})
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR )
  #MESSAGE(STATUS "LLVM include dir: " ${LLVM_INCLUDE_DIR})
  
  INCLUDE(CheckIncludeFileCXX)
  set(CMAKE_REQUIRED_INCLUDES ${LLVM_INCLUDE_DIR})
  check_include_file_cxx("llvm/Support/TargetSelect.h" HAVE_SUPPORT_TARGETSELECT_H)
  unset(CMAKE_REQUIRED_INCLUDES)
  MESSAGE(STATUS "HAVE_SUPPORT_TARGETSELECT_H: " ${HAVE_SUPPORT_TARGETSELECT_H})
  IF(HAVE_SUPPORT_TARGETSELECT_H)
    ADD_DEFINITIONS(-DHAVE_SUPPORT_TARGETSELECT_H)
  ENDIF(HAVE_SUPPORT_TARGETSELECT_H)

  set(CMAKE_REQUIRED_INCLUDES ${LLVM_INCLUDE_DIR})
  set(CMAKE_REQUIRED_DEFINITIONS -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -std=c++11)
  check_include_file_cxx("llvm/IRBuilder.h" HAVE_IRBUILDER_H)
  unset(CMAKE_REQUIRED_INCLUDES)
  MESSAGE(STATUS "HAVE_IRBUILDER_H: " ${HAVE_IRBUILDER_H})
  IF(HAVE_IRBUILDER_H)
    ADD_DEFINITIONS(-DHAVE_IRBUILDER_H)
  ENDIF(HAVE_IRBUILDER_H)

  set(CMAKE_REQUIRED_INCLUDES ${LLVM_INCLUDE_DIR})
  check_include_file_cxx("llvm/DataLayout.h" HAVE_DATALAYOUT_H)
  check_include_file_cxx("llvm/IR/DataLayout.h" HAVE_IR_DATALAYOUT_H)
  check_include_file_cxx("llvm/IR/Verifier.h" HAVE_IR_VERIFIER_H)
  unset(CMAKE_REQUIRED_INCLUDES)
  MESSAGE(STATUS "HAVE_DATALAYOUT_H: " ${HAVE_DATALAYOUT_H})
  MESSAGE(STATUS "HAVE_IR_DATALAYOUT_H: " ${HAVE_IR_DATALAYOUT_H})
  MESSAGE(STATUS "HAVE_IR_VERIFIER_H: " ${HAVE_IR_VERIFIER_H})
  IF(HAVE_DATALAYOUT_H)
    ADD_DEFINITIONS(-DHAVE_DATALAYOUT_H)
  ENDIF(HAVE_DATALAYOUT_H)
  IF(HAVE_IR_DATALAYOUT_H)
    ADD_DEFINITIONS(-DHAVE_IR_DATALAYOUT_H)
  ENDIF(HAVE_IR_DATALAYOUT_H)
  IF(HAVE_IR_VERIFIER_H)
    ADD_DEFINITIONS(-DHAVE_IR_VERIFIER_H)
  ENDIF(HAVE_IR_VERIFIER_H)
  
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --cxxflags  OUTPUT_VARIABLE LLVM_COMPILE_FLAGS )
  MESSAGE(STATUS "LLVM CXX flags: " ${LLVM_COMPILE_FLAGS})
  exec_program(${LLVM_CONFIG_EXECUTABLE} ARGS --ldflags   OUTPUT_VARIABLE LLVM_LDFLAGS )
  MESSAGE(STATUS "LLVM LD flags: " ${LLVM_LDFLAGS})
  exec_program( ${LLVM_CONFIG_EXECUTABLE} ARGS --system-libs  OUTPUT_VARIABLE LLVM_SYSTEM_LIBS RETURN_VALUE LLVM_SYSTEM_LIBS_FAILED)
  if(LLVM_SYSTEM_LIBS_FAILED)
    SET(LLVM_SYSTEM_LIBS "")
  endif(LLVM_SYSTEM_LIBS_FAILED)
  FIND_LLVM_LIBS( ${LLVM_CONFIG_EXECUTABLE} "core ipa ipo instrumentation bitreader bitwriter linker" LLVM_LIBS_CORE_ONLY LLVM_LIBS_CORE_OBJECTS )
  SET(LLVM_LIBS_CORE ${LLVM_LIBS_CORE_ONLY} ${LLVM_SYSTEM_LIBS})
  UNSET(LLVM_LIBS_CORE_ONLY)
  UNSET(LLVM_SYSTEM_LIBS_FAILED)
  MESSAGE(STATUS "LLVM core libs: " ${LLVM_LIBS_CORE})
  IF(${LLVM_STRING_VERSION} VERSION_GREATER 3.5)
  IF(APPLE AND UNIVERSAL)
    FIND_LLVM_LIBS( ${LLVM_CONFIG_EXECUTABLE} "engine native x86 PowerPC ARM" LLVM_LIBS_JIT LLVM_LIBS_JIT_OBJECTS )
  ELSE(APPLE AND UNIVERSAL)
    FIND_LLVM_LIBS( ${LLVM_CONFIG_EXECUTABLE} "engine native" LLVM_LIBS_JIT LLVM_LIBS_JIT_OBJECTS )
  ENDIF(APPLE AND UNIVERSAL)
  ELSE(${LLVM_STRING_VERSION} VERSION_GREATER 3.5)
  IF(APPLE AND UNIVERSAL)
    FIND_LLVM_LIBS( ${LLVM_CONFIG_EXECUTABLE} "jit native x86 PowerPC ARM" LLVM_LIBS_JIT LLVM_LIBS_JIT_OBJECTS )
  ELSE(APPLE AND UNIVERSAL)
    FIND_LLVM_LIBS( ${LLVM_CONFIG_EXECUTABLE} "jit native" LLVM_LIBS_JIT LLVM_LIBS_JIT_OBJECTS )
  ENDIF(APPLE AND UNIVERSAL)
  ENDIF(${LLVM_STRING_VERSION} VERSION_GREATER 3.5)
  MESSAGE(STATUS "LLVM JIT libs: " ${LLVM_LIBS_JIT})
  MESSAGE(STATUS "LLVM JIT objs: " ${LLVM_LIBS_JIT_OBJECTS})
  
  if(LLVM_INCLUDE_DIR)
    set(LLVM_FOUND TRUE)
  endif(LLVM_INCLUDE_DIR)
  
  if(LLVM_FOUND)
    message(STATUS "Found LLVM: ${LLVM_INCLUDE_DIR}")
  else(LLVM_FOUND)
    if(LLVM_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find LLVM")
    endif(LLVM_FIND_REQUIRED)
  endif(LLVM_FOUND)

endif (LLVM_INCLUDE_DIR)


