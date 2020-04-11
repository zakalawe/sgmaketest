cmake_minimum_required (VERSION 3.0)

if(COMMAND cmake_policy)
   if(POLICY CMP0054)
       cmake_policy(SET CMP0054 NEW)
   endif()
   if(POLICY CMP0042)
       cmake_policy(SET CMP0042 NEW)
   endif()
   if(POLICY CMP0067)
       cmake_policy(SET CMP0067 NEW)
   endif()
   if(POLICY CMP0093)
       cmake_policy(SET CMP0093 NEW)
   endif()
endif()


message(STATUS "CMAKE Build type: ${CMAKE_BUILD_TYPE}")
# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

include (CheckFunctionExists)
include (CheckIncludeFile)
include (CheckLibraryExists)
#include (CheckCXXSourceCompiles)
include (CheckCXXCompilerFlag)
include (GenerateExportHeader)

# only relevant for building shared libs but let's set it regardless
set(CMAKE_OSX_RPATH 1)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")

# let's use & require C++11 - note these are only functional with CMake 3.1
# we do manual fallbacks for CMake 3.0 in the compilers section
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# read 'version' file into a variable (stripping any newlines or spaces)
file(READ version versionFile)
string(STRIP ${versionFile} SIMGEAR_VERSION)

project(SimGear VERSION ${SIMGEAR_VERSION} LANGUAGES C CXX)

# add a dependency on the version file
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS version)

set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)

# use simgear version also as the SO version (if building SOs)
SET(SIMGEAR_SOVERSION ${SIMGEAR_VERSION})

# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

#####################################################################################
# Configure library search paths
#####################################################################################

if (NOT MSVC)
option(SIMGEAR_SHARED   "Set to ON to build SimGear as a shared library/framework" OFF)
option(SYSTEM_EXPAT     "Set to ON to build SimGear using the system expat library" OFF)
option(SYSTEM_UDNS      "Set to ON to build SimGear using the system udns library" OFF)
else()
# Building SimGear DLLs is currently not supported for MSVC.
set(SIMGEAR_SHARED OFF)
# Using external 3rd party libraries is currently not supported for MSVC - it would require shared simgear (DLL).
set(SYSTEM_EXPAT OFF)
set(SYSTEM_UDNS OFF)
endif()

option(SIMGEAR_HEADLESS "Set to ON to build SimGear without GUI/graphics support" ON)
option(ENABLE_RTI       "Set to ON to build SimGear with RTI support" OFF)
option(ENABLE_GDAL      "Set to ON to build SimGear with GDAL support" OFF)
option(ENABLE_TESTS     "Set to OFF to disable building SimGear's test applications" OFF)
option(ENABLE_SOUND     "Set to OFF to disable building SimGear's sound support" ON)
option(USE_AEONWAVE     "Set to ON to use AeonWave instead of OpenAL" ON)
option(ENABLE_PKGUTIL   "Set to ON to build the sg_pkgutil application (default)" ON)
option(ENABLE_DNS       "Set to ON to use udns library and DNS service resolver" OFF)
option(ENABLE_SIMD      "Enable SSE/SSE2 support for compilers" ON)
option(ENABLE_SIMD_CODE	"Enable SSE/SSE2 support code for compilers" OFF)
option(ENABLE_OPENMP    "Enable OpenMP compiler support" OFF)

if (NOT ENABLE_SIMD AND ENABLE_SIMD_CODE)
  set(ENABLE_SIMD_CODE OFF)
endif()

include (DetectArch)

# until the fstream fix is applied and generally available in OSG,
# keep the compatability link option as the default
option(OSG_FSTREAM_EXPORT_FIXED "Set to ON if the osgDB fstream export patch is applied" OFF)

if (CMAKE_COMPILER_IS_GNUCXX OR CLANG)
  if (CMAKE_VERSION VERSION_LESS 3.1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++${CMAKE_CXX_STANDARD}")
  elseif (CMAKE_VERSION VERSION_LESS 3.8)
    # policy CMP0067 (try_compile does not honor CMAKE_CXX_STANDARD)
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++${CMAKE_CXX_STANDARD}")
  endif()
endif()


if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
endif()

find_package(Boost REQUIRED)
set (BOOST_CXX_FLAGS "-DBOOST_BIMAP_DISABLE_SERIALIZATION")
include(BoostTestTargets)

find_package(ZLIB 1.2.4 REQUIRED)
find_package(CURL REQUIRED)

if (SYSTEM_EXPAT)
    message(STATUS "Requested to use system Expat library, forcing SIMGEAR_SHARED to true")
    set(SIMGEAR_SHARED ON)
    find_package(EXPAT REQUIRED)

else()
    message(STATUS "Using built-in expat code")
    set(EXPAT_INCLUDE_DIRS
            ${PROJECT_SOURCE_DIR}/3rdparty/expat
            ${PROJECT_BINARY_DIR}/3rdparty/expat)
endif(SYSTEM_EXPAT)

check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(windows.h HAVE_WINDOWS_H)

if(HAVE_INTTYPES_H)
  # ShivaVG needs inttypes.h
  add_definitions(-DHAVE_INTTYPES_H)
endif()

check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(rint HAVE_RINT)
check_function_exists(mkdtemp HAVE_MKDTEMP)
check_function_exists(bcopy HAVE_BCOPY)
check_function_exists(mmap HAVE_MMAP)

if (NOT MSVC)
  check_function_exists(timegm HAVE_TIMEGM)
  if (NOT HAVE_TIMEGM)
    message(FATAL_ERROR "Non-Windows platforms must support timegm()")
  endif()
endif()

# if(HAVE_UNISTD_H)
#     set(CMAKE_REQUIRED_INCLUDES ${CMAKE_INCLUDE_PATH})
#     check_cxx_source_compiles(
#        "#include <unistd.h>
#         #if !defined(_POSIX_TIMERS) || (0 >= _POSIX_TIMERS)
#             #error clock_gettime is not supported
#         #endif

#         int main() { return 0; }
#         "
#         HAVE_CLOCK_GETTIME)
# endif(HAVE_UNISTD_H)

set(RT_LIBRARY "")
if(HAVE_CLOCK_GETTIME)
    check_library_exists(rt clock_gettime "" HAVE_RT)
    if(HAVE_RT)
        set(RT_LIBRARY rt)
    endif(HAVE_RT)
endif(HAVE_CLOCK_GETTIME)

set(DL_LIBRARY "")
# check_cxx_source_compiles(
#     "#include <dlfcn.h>
#     int main(void) {
#         return 0;
#     }
#     "
#     HAVE_DLFCN_H)

if(HAVE_DLFCN_H)
    check_library_exists(dl dlerror "" HAVE_DL)
    if(HAVE_DL)
        set(DL_LIBRARY "dl")
    endif()
endif()

SET(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "add a postfix, usually 'd' on windows")
SET(CMAKE_RELEASE_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
SET(CMAKE_RELWITHDEBINFO_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")
SET(CMAKE_MINSIZEREL_POSTFIX "" CACHE STRING "add a postfix, usually empty on windows")

# isnan might not be real symbol, so can't check using function_exists
# check_cxx_source_compiles(
#     "#include <cmath>
#     int main() { return std::isnan(0.0);} "
#     HAVE_STD_ISNAN)

# if (NOT ${HAVE_STD_ISNAN})
#   message(FATAL_ERROR "Your compiler lacks C++11 std::isnan, please update it")
# endif()

set(HAVE_STD_ISNAN 1)

# Check if the <regex> implementation in the C++ standard library is usable.
# This is necessary because g++ 4.8 lies about its C++11 compliance: its
# <regex> is utterly unusable, cf. [1].
# The big preprocessor test essentially comes from [2], and gcc upstream devs
# appear to back it (see comments following [2], as well as [3]).
#
#   [1] https://stackoverflow.com/a/12665408/4756009
#   [2] https://stackoverflow.com/a/41186162/4756009
#   [3] https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78905
# check_cxx_source_compiles(
#     "#include <regex>

#      int main() {
#        #if __cplusplus >= 201103L &&                             \
#            (!defined(__GLIBCXX__)                         ||     \
#             (__cplusplus >= 201402L)                      ||     \
#             defined(_GLIBCXX_REGEX_DFS_QUANTIFIERS_LIMIT) ||     \
#             defined(_GLIBCXX_REGEX_STATE_LIMIT)           ||     \
#             (defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE > 4))
#        #else
#          nullptr = void; // intentionally trigger a compilation error
#        #endif
#      }"
#     HAVE_WORKING_STD_REGEX)

  
set(HAVE_WORKING_STD_REGEX 1)

# if(CMAKE_COMPILER_IS_GNUCXX)
#     set(WARNING_FLAGS_CXX "-Wall -fPIC")
#     set(WARNING_FLAGS_C   "-Wall -fPIC")

#     if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4)
#       message(WARNING "GCC 4.4 will be required soon, please upgrade")
#     endif()

#     if (CMAKE_BUILD_TYPE STREQUAL "Debug")
#       set(CMAKE_C_FLAGS
#         "${CMAKE_C_FLAGS} -O0 -fno-omit-frame-pointer -fno-inline")
#       set(CMAKE_CXX_FLAGS
#         "${CMAKE_CXX_FLAGS} -O0 -fno-omit-frame-pointer -fno-inline")
#     elseif (ENABLE_SIMD)
#       if (X86 OR X86_64)
#         set(CMAKE_C_FLAGS_RELEASE "-O3 -msse2 -mfpmath=sse -ftree-vectorize -ftree-slp-vectorize")
#         set(CMAKE_CXX_FLAGS_RELEASE "-O3 -msse2 -mfpmath=sse -ftree-vectorize -ftree-slp-vectorize")
#       endif()
#     endif()

#     # certain GCC versions don't provide the atomic builds, and hence
#     # require is to provide them in SGAtomic.cxx
#     set(CMAKE_REQUIRED_INCLUDES ${CMAKE_INCLUDE_PATH})
#     check_cxx_source_compiles(
#        "int main() { unsigned mValue; return __sync_add_and_fetch(&mValue, 1); }"
#         GCC_ATOMIC_BUILTINS_FOUND)
# endif(CMAKE_COMPILER_IS_GNUCXX)

if (CLANG)
    # Boost redeclares class members
    set(WARNING_FLAGS_CXX "-Wall -fPIC -Wno-overloaded-virtual -Wno-redeclared-class-member")
    set(WARNING_FLAGS_C   "-Wall -fPIC")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
    # fix Boost compilation :(
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -O0 -fno-omit-frame-pointer -fno-inline-functions")
      set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -O0 -fno-omit-frame-pointer -fno-inline-functions")
    elseif (ENABLE_SIMD)
      if (X86 OR X86_64)
        set(CMAKE_C_FLAGS_RELEASE "-O3 -msse2 -mfpmath=sse -ftree-vectorize -ftree-slp-vectorize")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -msse2 -mfpmath=sse -ftree-vectorize -ftree-slp-vectorize")
      endif()
    endif()
endif()


if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
# boost goes haywire wrt static asserts
    check_cxx_compiler_flag(-Wno-unused-local-typedefs HAS_NOWARN_UNUSED_TYPEDEFS)
    if(HAS_NOWARN_UNUSED_TYPEDEFS)
        set(WARNING_FLAGS_CXX " ${WARNING_FLAGS_CXX} -Wno-unused-local-typedefs")
    endif()
endif()



set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS_C} ${MSVC_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS_CXX} ${MSVC_FLAGS} ${BOOST_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MSVC_LD_FLAGS}")

#include(CheckCXXFeatures)

# use BEFORE to ensure local directories are used first,
# ahead of system-installed libs
include_directories(BEFORE ${PROJECT_BINARY_DIR}/simgear)

add_definitions(-DHAVE_CONFIG_H)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/simgear/simgear_config_cmake.h.in"
  "${PROJECT_BINARY_DIR}/simgear/simgear_config.h"
  )


install (FILES ${PROJECT_BINARY_DIR}/simgear/simgear_config.h  DESTINATION include/simgear/)

#add_subdirectory(3rdparty)
add_subdirectory(simgear)
