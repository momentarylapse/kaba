# created by xmake

cmake_minimum_required(VERSION 3.21)
project(mouduleterminal)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)


#=======================================================================================
#    libraries
#=======================================================================================

find_package(PkgConfig REQUIRED)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND DEPS Threads::Threads)


#=======================================================================================
#    target moduleterminal
#=======================================================================================

add_library(moduleterminal SHARED
	src/lib/base/array.cpp
	src/lib/base/pointer.cpp
	src/lib/base/strings.cpp
	src/main.cpp)
target_include_directories(moduleterminal PUBLIC ${INCLUDE_DIRECTORIES})
target_link_directories(moduleterminal PUBLIC ${LINK_DIRECTORIES})
target_compile_options(moduleterminal PUBLIC ${COMPILE_OPTIONS})
target_compile_definitions(moduleterminal PUBLIC ${COMPILE_DEFINITIONS}  INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
target_link_libraries(moduleterminal PUBLIC ${DEPS})
if (NOT WIN32)
	target_compile_options(moduleterminal PUBLIC -fvisibility=hidden)
endif()
target_include_directories(moduleterminal PUBLIC src)
target_include_directories(moduleterminal PUBLIC src/lib)


#=======================================================================================
#    install
#=======================================================================================

set(RESOURCE_DESTINATION share/mouduleterminal)

# binaries	

# resources
install(DIRECTORY "static" DESTINATION ${RESOURCE_DESTINATION})