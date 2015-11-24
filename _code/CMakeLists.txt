# Copyright Louis Dionne 2015
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

##############################################################################
# Setup project
##############################################################################
project(ldionne.com CXX)
cmake_minimum_required(VERSION 3.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_BINARY_DIR}/modules")
enable_testing()


##############################################################################
# Setup compiler flags (more can be set on a per-target basis or in subdirectories)
##############################################################################
include(CheckCXXCompilerFlag)
macro(append_flag testname flag)
    check_cxx_compiler_flag(${flag} ${testname})
    if (${testname})
        add_compile_options(${flag})
    endif()
endmacro()

append_flag(HAS_FTEMPLATE_BACKTRACE_LIMIT  -ftemplate-backtrace-limit=0)
append_flag(HAS_PEDANTIC                   -pedantic)
append_flag(HAS_STDCXX1Y                   -std=c++1y)
append_flag(HAS_W                          -W)
append_flag(HAS_WALL                       -Wall)
append_flag(HAS_WEXTRA                     -Wextra)
append_flag(HAS_WNO_UNUSED_LOCAL_TYPEDEFS  -Wno-unused-local-typedefs)
append_flag(HAS_WWRITE_STRINGS             -Wwrite-strings)

# By default, we enable the GNU extension allowing the special string literal
# operator template, which enables the _s suffix for creating compile-time
# Strings. We do so to allow testing this operator. We also suppress warnings
# pertaining to this operator.
add_definitions(-DBOOST_HANA_CONFIG_ENABLE_STRING_UDL)
append_flag(HAS_WNO_GNU_STRING_UDL -Wno-gnu-string-literal-operator-template)


##############################################################################
# Search for packages.
#
# Behavior when the package is found or not is customized at the
# point where the package is required.
##############################################################################
find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS})

file(DOWNLOAD "https://raw.githubusercontent.com/boostorg/hana/master/cmake/FindHana.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/modules/FindHana.cmake")
set(Hana_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/ext/hana)
find_package(Hana 0.6.0)
include_directories(${Hana_INCLUDE_DIRS})

file(DOWNLOAD "https://raw.githubusercontent.com/boostorg/hana/master/cmake/FindMeta.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/modules/FindMeta.cmake")
set(Meta_CLONE_IF_MISSING ON)
find_package(Meta)
include_directories(${Meta_INCLUDE_DIR})


##############################################################################
# Setup subdirectories
##############################################################################
# add_subdirectory(revisiting-the-tiny-mpl)
