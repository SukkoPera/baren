cmake_minimum_required (VERSION 2.6)

project (Baren)

#~ find_library (PCRE_LIBRARY pcre)
#~ find_path (GLIB_INCLUDE_DIR NAMES glib.h PATH_SUFFIXES glib-2.0)
include (FindGLIB.cmake)


include_directories (${GLIB_INCLUDES})

add_executable (
	baren
	
	baren.c
)

target_link_libraries (
	baren
	
	${GLIB_LIBRARIES}
)
