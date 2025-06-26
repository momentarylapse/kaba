# created by xmake

cmake_minimum_required(VERSION 3.21)
project(moudulemongo)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)


#=======================================================================================
#    libraries
#=======================================================================================

find_package(PkgConfig REQUIRED)



# lib: bson (mandatory)
pkg_check_modules(LIBBSON10 REQUIRED libbson-1.0)
list(APPEND INCLUDE_DIRECTORIES ${LIBBSON10_INCLUDE_DIRS})
list(APPEND LINK_DIRECTORIES ${LIBBSON10_LIBRARY_DIRS})
list(APPEND DEPS ${LIBBSON10_LIBRARIES})
list(APPEND COMPILE_OPTIONS ${LIBBSON10_CFLAGS_OTHER})
list(APPEND COMPILE_DEFINITIONS HAS_LIB_BSON=1)


# lib: mongo (mandatory)
pkg_check_modules(LIBMONGOC10 REQUIRED libmongoc-1.0)
list(APPEND INCLUDE_DIRECTORIES ${LIBMONGOC10_INCLUDE_DIRS})
list(APPEND LINK_DIRECTORIES ${LIBMONGOC10_LIBRARY_DIRS})
list(APPEND DEPS ${LIBMONGOC10_LIBRARIES})
list(APPEND COMPILE_OPTIONS ${LIBMONGOC10_CFLAGS_OTHER})
list(APPEND COMPILE_DEFINITIONS HAS_LIB_MONGO=1)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND DEPS Threads::Threads)


#=======================================================================================
#    target modulemongo
#=======================================================================================

add_library(modulemongo SHARED
	src/lib/base/array.cpp
	src/lib/base/pointer.cpp
	src/lib/base/strings.cpp
	src/main.cpp)
target_include_directories(modulemongo PUBLIC ${INCLUDE_DIRECTORIES})
target_link_directories(modulemongo PUBLIC ${LINK_DIRECTORIES})
target_compile_options(modulemongo PUBLIC ${COMPILE_OPTIONS})
target_compile_definitions(modulemongo PUBLIC ${COMPILE_DEFINITIONS}  INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
target_link_libraries(modulemongo PUBLIC ${DEPS})
if (NOT WIN32)
	target_compile_options(modulemongo PUBLIC -fvisibility=hidden)
endif()
target_include_directories(modulemongo PUBLIC src)
target_include_directories(modulemongo PUBLIC src/lib)


#=======================================================================================
#    install
#=======================================================================================

set(RESOURCE_DESTINATION share/moudulemongo)

# binaries	

# resources
install(DIRECTORY "static" DESTINATION ${RESOURCE_DESTINATION})