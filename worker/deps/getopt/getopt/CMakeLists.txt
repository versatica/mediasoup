PROJECT(wingetopt)
cmake_minimum_required(VERSION 2.8)

option(BUILD_SHARED_LIBS "Build the shared library" OFF)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

if(BUILD_SHARED_LIBS)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  add_definitions(-DBUILDING_WINGETOPT_DLL -DWINGETOPT_SHARED_LIB)
endif()

add_library(wingetopt src/getopt.c src/getopt.h)

install(FILES src/getopt.h DESTINATION include)

install(TARGETS wingetopt
    RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)

