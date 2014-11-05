


# root cmake file

# todo
# gtest in das co.. - Projekt direkt mit einbinden 

# install created stuff and add dependen libraries, especially for qt5:
# https://code.google.com/p/glmixer/source/browse/trunk/src/CMakeLists.txt notwendige 

# Minimal version
cmake_minimum_required (VERSION 3.0)
# What we are
project (coasync4cpp)

####### public build options   

# BUILD_SHARED_LIBS is a standard CMake variable, but we declare it here to
# make it prominent in the GUI.
option(BUILD_SHARED_LIBS "Build shared libraries (DLLs)." OFF)

option(build_coasync4qt "Build qt specific library, requires at least qt 5.0." OFF)
option(build_tests "Build coasync specific unit-tests." OFF)

####### global settings   

# our own tools directory 
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_CXX_COMPILER_ID}" )
set ( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_CXX_COMPILER_ID}" )
set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_CXX_COMPILER_ID}" )	

include (utils)

# Platform dependent defines
if ( ${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC" ) 
    # /SAFESEH:NO helps to overcome compatibility issues with precompiled boost libraries:
	set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO"  )
	message ( STATUS "\t output linker flags  ${CMAKE_EXE_LINKER_FLAGS} ..." )
endif()
	
message ( STATUS "\t output dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ..." )
message ( STATUS "\t system: ${CMAKE_SYSTEM_NAME} " )

####### boost  
# we need to configure boost here, because of the automatic linker option on windows.
# for this option we need to add the boost library path to the library search paths.
 
set(BOOST_MIN_VERSION "1.55.0")
add_definitions( -DBOOST_THREAD_VERSION=4 )

set( Boost_USE_STATIC_LIBS        ON)
set( Boost_USE_MULTITHREADED      ON)
set( Boost_USE_STATIC_RUNTIME    OFF)

find_package (Boost ${BOOST_MIN_VERSION} REQUIRED)

if (NOT Boost_FOUND)
      message(FATAL_ERROR "Fatal error: Boost (version >= ${BOOST_MIN_VERSION}) required.\n")
endif (NOT Boost_FOUND)

link_directories (${Boost_LIBRARY_DIR})

####### coasync4cpp 

message ( STATUS "\tcreate coasync4cpp..." )
add_subdirectory (coasync4cpp)
project_group( coasync4cpp   "Libs")

####### coasync4qt 

if (build_coasync4qt)

	message ( STATUS "\tcreate coasync4qt..." )
	add_subdirectory (coasync4qt)
    project_group( coasync4qt   "Libs")

endif()

####### if tests are requested ... 

if (build_tests)
	set( BUILD_SHARED_LIBS ON )
	set( gtest_build_samples OFF )
	add_subdirectory(gtest-1.7.0)
	include_directories( ${CMAKE_SOURCE_DIR}/gtest-1.7.0/include )
    project_group( gtest   "Tests")
    project_group( gtest_main   "Tests")
endif()

if (build_tests)
	add_subdirectory(testcoasync4cpp)
    project_group( testcoasync4cpp   "Tests")
endif()

if (build_tests )
if (build_coasync4qt)
	add_subdirectory(testcoasync4qt)
	project_group( testcoasync4qt "Tests" )
endif()
endif()

