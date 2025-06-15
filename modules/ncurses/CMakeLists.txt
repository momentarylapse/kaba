# created by xmake

cmake_minimum_required(VERSION 3.21)
project(moudulencurses)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)


#=======================================================================================
#    libraries
#=======================================================================================

find_package(PkgConfig REQUIRED)



# lib: ncurses (mandatory)
pkg_check_modules(NCURSES REQUIRED ncurses)
list(APPEND INCLUDE_DIRECTORIES ${NCURSES_INCLUDE_DIRS})
list(APPEND LINK_DIRECTORIES ${NCURSES_LIBRARY_DIRS})
list(APPEND DEPS ${NCURSES_LIBRARIES})
list(APPEND COMPILE_OPTIONS ${NCURSES_CFLAGS_OTHER})
list(APPEND COMPILE_DEFINITIONS HAS_LIB_NCURSES=1)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND DEPS Threads::Threads)


#=======================================================================================
#    target modulencurses
#=======================================================================================

add_library(modulencurses SHARED
	src/lib/base/array.cpp
	src/lib/base/pointer.cpp
	src/lib/base/strings.cpp
	src/main.cpp)
target_include_directories(modulencurses PUBLIC ${INCLUDE_DIRECTORIES})
target_link_directories(modulencurses PUBLIC ${LINK_DIRECTORIES})
target_compile_options(modulencurses PUBLIC ${COMPILE_OPTIONS})
target_compile_definitions(modulencurses PUBLIC ${COMPILE_DEFINITIONS}  INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
target_link_libraries(modulencurses PUBLIC ${DEPS})
if (NOT WIN32)
	target_compile_options(modulencurses PUBLIC -fvisibility=hidden)
endif()
target_include_directories(modulencurses PUBLIC src)
target_include_directories(modulencurses PUBLIC src/lib)


#=======================================================================================
#    install
#=======================================================================================

set(RESOURCE_DESTINATION share/moudulencurses)

# binaries	

# resources
install(DIRECTORY "static" DESTINATION ${RESOURCE_DESTINATION})