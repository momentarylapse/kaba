# created by xmake

cmake_minimum_required(VERSION 3.21)
project(moudulesqlite)
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



# lib: sqlite (mandatory)
find_package(SQLite3 REQUIRED)
list(APPEND DEPS SQLite::SQLite3)
list(APPEND COMPILE_DEFINITIONS HAS_LIB_SQLITE=1)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND DEPS Threads::Threads)


#=======================================================================================
#    target modulesqlite
#=======================================================================================

add_library(modulesqlite SHARED
	src/lib/base/array.cpp
	src/lib/base/pointer.cpp
	src/lib/base/strings.cpp
	src/main.cpp)
target_include_directories(modulesqlite PUBLIC ${INCLUDE_DIRECTORIES})
target_link_directories(modulesqlite PUBLIC ${LINK_DIRECTORIES})
target_compile_options(modulesqlite PUBLIC ${COMPILE_OPTIONS})
target_compile_definitions(modulesqlite PUBLIC ${COMPILE_DEFINITIONS}  INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
target_link_libraries(modulesqlite PUBLIC ${DEPS})
if (NOT WIN32)
	target_compile_options(modulesqlite PUBLIC -fvisibility=hidden)
endif()
target_include_directories(modulesqlite PUBLIC src)
target_include_directories(modulesqlite PUBLIC src/lib)


#=======================================================================================
#    install
#=======================================================================================

set(RESOURCE_DESTINATION share/moudulesqlite)

# binaries	

# resources
install(DIRECTORY "static" DESTINATION ${RESOURCE_DESTINATION})