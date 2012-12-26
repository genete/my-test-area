project(cairo-sample)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_PREFIX_PATH "/Users/carlos/Software/cairo")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK2 REQUIRED gtk+-2.0)

include_directories(${GTK2_INCLUDE_DIRS})
link_directories(${GTK2_LIBRARY_DIRS})
add_definitions(${GTK2_CFLAGS_OTHER})

find_library(CAIRO_LIBRARY
			 NAMES cairo
			 HINTS "${CMAKE_PREFIX_PATH}/lib"
			 )
find_path(CAIRO_GL_INCLUDE_DIRS cairo-gl.h
		  ${CMAKE_PREFIX_PATH}/include/cairo
		  )

include_directories(BEFORE ${CAIRO_GL_INCLUDE_DIRS})

add_executable(cairo-sample cairo_sample.c)
target_link_libraries(cairo-sample ${CAIRO_LIBRARY} GL ${GTK2_LIBRARIES})
