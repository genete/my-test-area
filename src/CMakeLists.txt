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

#set(CAIRO_INCLUDE_DIRS "/Users/carlos/Software/cairo/include/cairo")
#set(CAIRO_LIBRARY_DIRS "/Users/carlos/Software/cairo/lib")
#set(CAIRO_LIBRARIES cairo GL)

include_directories(BEFORE ${CAIRO_GL_INCLUDE_DIRS})
#link_directories(${CAIRO_LIBRARY_DIRS})

#message("Cairo Include: ${CAIRO_INCLUDE_DIRS}")
#message("Cairo Libraries: ${CAIRO_LIBRARIES}")
#message("Cairo Library dirs: ${CAIRO_LIBRARY_DIRS}")
#message("----")
#message("gtk2 include dir: ${GTK2_INCLUDE_DIRS}")
#message("gtk2 library dir: ${GTK2_LIBRARY_DIRS}")
#message("gtk2 libraries: ${GTK2_LIBRARIES}")
#message("gtk2 flags others: ${GTK2_CFLAG_OTHER}")

add_executable(cairo-sample cairo_sample.c)
target_link_libraries(cairo-sample ${CAIRO_LIBRARY} GL ${GTK2_LIBRARIES})
