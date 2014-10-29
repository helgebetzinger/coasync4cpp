
include (utils)

####### we need Qt5Core 

find_package(Qt5Core REQUIRED)
if (NOT Qt5Core_FOUND)
	  message(FATAL_ERROR "Fatal error: Qt5Core (version >= 5.0) required.\n")
endif (NOT Qt5Core_FOUND)

include_directories( ${Qt5Core_INCLUDE_DIRS} )

set(CMAKE_AUTOMOC ON)
make_filelist ("src/" build_sources build_headers)

# Create a library called "coasync4cpp"
add_library (coasync4qt ${LIBRARY_FLAGS} ${build_sources} ${build_headers})

# Make sure the compiler can find include files for our coasync4qt library
# when other libraries or executables link to coasync4qt
target_include_directories (coasync4qt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src" ${Qt5Core_INCLUDE_DIRS} )
# add necessary dependencies to this library
target_link_libraries( coasync4qt PUBLIC coasync4cpp ${Qt5Core_LIBRARIES} ) 
