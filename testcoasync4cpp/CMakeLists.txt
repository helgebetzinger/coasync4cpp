
include (utils)

make_filelist ("src/" build_sources build_headers)

# Create a library called "coasync4cpp"
add_executable (testcoasync4cpp ${LIBRARY_FLAGS} ${build_sources} ${build_headers})

# add necessary dependencies to this library
target_link_libraries( testcoasync4cpp coasync4cpp gtest_main ) 
