# created by xmake

cmake_minimum_required(VERSION 3.21)
project(moudulezlib)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)


#=======================================================================================
#    libraries
#=======================================================================================

find_package(PkgConfig REQUIRED)



# lib: dl (optional)
if(WIN32)
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_DL=0)
else()
	list(APPEND DEPS ${CMAKE_DL_LIBS})
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_DL=1)
endif()



# lib: zlib (mandatory)
find_package(ZLIB REQUIRED)
list(APPEND DEPS ZLIB::ZLIB)
list(APPEND COMPILE_DEFINITIONS HAS_LIB_ZLIB=1)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND DEPS Threads::Threads)


#=======================================================================================
#    target modulezlib
#=======================================================================================

add_library(modulezlib SHARED
	src/lib/base/array.cpp
	src/lib/base/pointer.cpp
	src/lib/base/strings.cpp
	src/main.cpp)
target_include_directories(modulezlib PUBLIC ${INCLUDE_DIRECTORIES})
target_link_directories(modulezlib PUBLIC ${LINK_DIRECTORIES})
target_compile_options(modulezlib PUBLIC ${COMPILE_OPTIONS})
target_compile_definitions(modulezlib PUBLIC ${COMPILE_DEFINITIONS}  INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
target_link_libraries(modulezlib PUBLIC ${DEPS})
if (NOT WIN32)
	target_compile_options(modulezlib PUBLIC -fvisibility=hidden)
endif()
target_include_directories(modulezlib PUBLIC src)
target_include_directories(modulezlib PUBLIC src/lib)


#=======================================================================================
#    install
#=======================================================================================

set(RESOURCE_DESTINATION share/moudulezlib)

# binaries	

# resources
install(DIRECTORY "static" DESTINATION ${RESOURCE_DESTINATION})