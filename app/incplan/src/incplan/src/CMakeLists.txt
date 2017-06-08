cmake_minimum_required(VERSION 2.8.12)
project(incplan)

# Options
set(USER_CXX_FLAGS "" CACHE STRING "Use this variable to provide additional\
flags to the compiler.")
set(IPASIR_DIR "${CMAKE_SOURCE_DIR}/../lib/ipasir" CACHE STRING
"The directory which contains one or more implemntations to the ipasir \
interface. The libs should have the format 'libipasir[solver-name].a'")
set(BUILD_TESTS "ON" CACHE STRING
"Whether to build tests. Values: ON, OFF")

# Basic Settings
set(CMAKE_CXX_FLAGS "-std=c++14 -O3 -Wall -Wextra -Wpedantic -pthread -g ${USER_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../bin)
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/libs)

set(UNIT_TEST_FILES
	unit_tests/TestTimeSlotMapping.cpp
	unit_tests/TestTimePointBasedSolver.cpp)

set(SRC_FILES
	incplan.cpp
	TimeSlotMapping.cpp
	carj/carj.cpp
	)


# Include cotire to precompile headers.
# We don't need cotire, but it might speed up build process as we have header
# only libraries.
#include(cotire)
#set (COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES 1)

add_library(ipasir_wrapper libs/ipasir/ipasir_cpp.cpp)
add_library(ipasir_wrapper_extended libs/ipasir/ipasir_cpp.cpp)
target_compile_definitions(ipasir_wrapper_extended PRIVATE USE_EXTENDED_IPASIR=0)

# === Target: all_sources ===

# This library contains every source file. We pack them into a library as we
# will most likely link against multiple ipasir solvers and do not want to
# compile the sources again for each solver
add_library(all_sources ${SRC_FILES})
set_target_properties(all_sources PROPERTIES
	COTIRE_PREFIX_HEADER_IGNORE_PATH "${CMAKE_SOURCE_DIR}"
		_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/libs")
#cotire(all_sources)

# === Target incplan-debugprint ===
add_library(libincplan_debugprint ipasirprint.cpp)
add_executable(incplan_debugprint main.cpp)
target_link_libraries(incplan_debugprint
	all_sources
	ipasir_wrapper
	libincplan_debugprint)


if (BUILD_TESTS EQUAL "ON")
	# Include google test, our testing framework
	include(gtest)

	# === Target: unitTest ===

	add_executable(unitTest ${UNIT_TEST_FILES})
	target_link_libraries(unitTest
		all_sources
		ipasir_wrapper
		gtest_main
		gmock_main
		libincplan_debugprint)
	set_target_properties(unitTest PROPERTIES
		COTIRE_PREFIX_HEADER_IGNORE_PATH "${CMAKE_SOURCE_DIR}"
			_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/libs")
	#cotire(unitTest)
else()
	add_custom_target(unitTest)
endif()

# === Target: runtest ===
add_custom_target(runTest
	COMMAND ./unitTest --gtest_color=yes
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
	DEPENDS unitTest)

# === Target: incplan-heuristical ===

# A target that uses not the default heuristic of the solver but our own.

set(EXTENDED_IPASIR_SOLVER ${CMAKE_SOURCE_DIR}/../lib/libipasirExt_cominisatps2sun_nopre.a)
if (EXISTS ${EXTENDED_IPASIR_SOLVER})
	add_executable(incplan-heuristical main.cpp)
	target_link_libraries(incplan-heuristical
		all_sources
		ipasir_wrapper_extended
		${EXTENDED_IPASIR_SOLVER}
		)
endif()


# === Target: incplan-[solver_name] ===

# Creates executables for each aviable solver [solver-name]

# This folder should contain the library of solvers to link aigainst. Each
# incremental solver library should be named libipasir[solver-name].a
get_filename_component(IPASIR_DIR_ABS ${IPASIR_DIR} ABSOLUTE)
file(GLOB ipasir_libs RELATIVE ${IPASIR_DIR_ABS}
	${IPASIR_DIR_ABS}/libipasir*.a
	)

FOREACH(ipasir_lib ${ipasir_libs})
	string(REGEX REPLACE "libipasir(.*).a" "\\1" libname ${ipasir_lib})
	add_executable(incplan-${libname} main.cpp)
	target_link_libraries(incplan-${libname}
		all_sources
		ipasir_wrapper
		${IPASIR_DIR_ABS}/${ipasir_lib}
		)
ENDFOREACH()

# === Target: core ===

# Dummy target which builds all targets but only for one solver
add_custom_target(core DEPENDS incplan_debugprint unitTest runTest)
if (TARGET incplan-cominisatps2sun_nopre)
	add_dependencies(core incplan-cominisatps2sun_nopre)

	# Make sure the tool is build, even if tests fail.
	add_dependencies(runTest incplan-cominisatps2sun_nopre)
endif()
if (TARGET incplan-heuristical)
	add_dependencies(core incplan-heuristical)

	# Make sure the tool is build, even if tests fail.
	add_dependencies(runTest incplan-heuristical)
endif()