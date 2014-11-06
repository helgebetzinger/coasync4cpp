
include (utils)

####### boost  

find_package (Boost ${BOOST_MIN_VERSION} COMPONENTS context thread coroutine system REQUIRED)

####### setup the library itself 

make_filelist ("src/" build_sources build_headers )

# Create a library called "coasync4cpp"
add_library (coasync4cpp ${LIBRARY_FLAGS} ${build_sources} ${build_headers})

# Make sure the compiler can find include files for our coasync4cpp library
# when other libraries or executables link to Hello
target_include_directories (coasync4cpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src" ${Boost_INCLUDE_DIRS} )
target_link_libraries( coasync4cpp PUBLIC ${Boost_LIBRARIES} rt )

if (UNIX)
    # need on unix librt for linking boost thread:
	target_link_libraries( coasync4cpp PUBLIC rt )
endif() 
