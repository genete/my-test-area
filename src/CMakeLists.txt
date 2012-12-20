project(cairo-sample)
cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
pkg_check_modules(CAIRO REQUIRED cairo)

include_directories(${GTK2_INCLUDE_DIRS})
link_directories(${GTK2_LIBRARY_DIRS})
add_definitions(${GTK2_CFLAGS_OTHER})

#include_directories(${CAIRO_INCLUDE_DIRS})
message("${CAIRO_INCLUDE_DIRS}")

add_executable(cairo-sample cairo2.c)
target_link_libraries(cairo-sample ${CAIRO_LIBRARIES} ${GTK2_LIBRARIES})
