
include (utils)

find_package(Qt5Concurrent REQUIRED)
if (NOT Qt5Concurrent_FOUND)
	  message(FATAL_ERROR "Fatal error: Qt5Concurrent (version >= 5.0) required.\n")
endif (NOT Qt5Concurrent_FOUND)

include_directories( ${Qt5Concurrent_INCLUDE_DIRS} )

set(CMAKE_AUTOMOC ON)
make_filelist ("src/" build_sources build_headers)

add_executable (testcoasync4qt ${LIBRARY_FLAGS} ${build_sources} ${build_headers})

# add necessary dependencies to this executable
target_link_libraries( testcoasync4qt coasync4qt ${Qt5Concurrent_LIBRARIES} gtest_main ) 
