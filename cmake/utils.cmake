# Generates a standard module file list (from all inside src/)
# See macro make_filelist
 
# Generates a list (output) of all elements in input which match the regexpression "pattern"
# correct call:
# list_regex ("${sources}" unix_sources ".*/unix/.*")
macro (list_regex input output pattern)
#message ("Input ${input}")
#message ("Pattern ${pattern}")
	foreach (i ${input})
		if (${i} MATCHES "${pattern}")			
			list (APPEND ${output} ${i})
		endif()
	endforeach()
endmacro (list_regex)

# adding ObjC/ObjC++ Flags as cmake currently does not support this automatically
# OBJC_FLAGS and OBJCXX_FLAGS must be set by the main CMakeLists.txt file
# make_filelist will use it automatically.
# call it this way fix_objc_flags ("${source_list}")
macro (fix_objc_flags sources)
if (MAC_OSX)
		if (OBJC_FLAGS)
			list_regex ("${sources}" objc_sources ".*[.]m$")
			set_source_files_properties (${objc_sources} PROPERTIES COMPILE_FLAGS ${OBJC_FLAGS})
		endif()
		if (OBJCXX_FLAGS)
			list_regex ("${sources}" objcxx_sources ".*[.]mm$")
			set_source_files_properties (${objcxx_sources} PROPERTIES COMPILE_FLAGS ${OBJCXX_FLAGS})
		endif()
		# message ("Sources:  ${sources}")
		# message (".m  files: ${objc_sources}")
		# message (".mm files: ${objcxx_sources}")
	endif()
endmacro (fix_objc_flags)

# put the visual studio groups right
# also see www.mail-archive.com/cmake@cmake.org/msg10171.html
macro (group_them prefix sources)
	foreach (file ${${sources}})
		# get full location
		get_filename_component (path ${file} PATH)
		
		# guessing a groupname
		string (REPLACE ${CMAKE_CURRENT_SOURCE_DIR} "" group ${path})
		

		# remove beginning with "/" or "\\"
		string (SUBSTRING ${group} 0 1 firstChar)
		if (firstChar STREQUAL "/" OR firstChar STREQUAL "//")
			string (LENGTH ${group} len)
			math (EXPR len "${len}-1")
			string (SUBSTRING ${group} 1 ${len} group)
		endif()

		if (MAC_OSX)
			# OSX XCode Generator has a bug and cannot display groups which are working in WIN32
			# Bug: http://public.kitware.com/Bug/view.php?id=7932
			# So we exchange "/" with " " to get at least some grouping
			# string (REPLACE "/" " " group ${group})
		else ()
			# exchanging "/" with "//"
			string (REPLACE "/" "\\" group ${group})
		endif()

		
		# adding group name
		set (group "${prefix}_${group}")
		source_group (${group} FILES ${file})
		#message ("Path: ${path} Group: ${group}          of file ${file}")	
	endforeach() 
endmacro (group_them)


# generates file lists of c++ standard files inside the folder baseDir
# source file list will be saved in build_sources
# header file list will be saved in build_headers
macro (make_filelist baseDir build_sources build_headers)
	file (GLOB_RECURSE sources ${baseDir}/*.cpp ${baseDir}/*.c ${baseDir}/*.mm ${baseDir}/*.m ${baseDir}/*.h) ## .mm/.m is ObjC code on the mac
	
	list_regex ("${sources}" unix_sources ".*/unix/.*")
	list_regex ("${sources}" linux_sources ".*/linux/.*")
	list_regex ("${sources}" win32_sources ".*/win32/.*")
	list_regex ("${sources}" macosx_sources ".*/mac_osx/.*")
	set (common_sources ${sources})
	list (REMOVE_ITEM common_sources "gueranteednotexistingelement" ${unix_sources} ${linux_sources} ${win32_sources} ${macosx_sources}) # guaranteed... is a hack because this task kills everything if the platform sources are empty
	
	#message ("sources  		 ${sources}")
	#message ("win32 sources  ${win32_sources}")
	
	if (WIN32)
		list (APPEND ${build_sources} ${common_sources} ${win32_sources})
	elseif (MAC_OSX)
		list (APPEND ${build_sources} ${common_sources} ${macosx_sources} ${unix_sources})
	elseif (LINUX)
		list (APPEND ${build_sources} ${common_sources} ${linux_sources} ${unix_sources})
	endif()
	list (SORT ${build_sources}) # Helps XCode displaying the right packages
	
	# cannot be inside MAC_OSX zone, as some targets (e.g. mac_osx client) doesn't mark osx files explizit.
	fix_objc_flags ("${macosx_sources}")
	
	# group them right (for win32/visualstudio)
	group_them ("" "${build_sources}")
	
endmacro (make_filelist)

macro ( project_group target_name target_group ) 
	# Organize projects into folders, @see http://athile.net/library/blog/?p=288 
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	set_property( TARGET ${target_name}  PROPERTY FOLDER ${target_group} )
endmacro ( project_group ) 
