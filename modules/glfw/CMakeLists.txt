# created by xmake

cmake_minimum_required(VERSION 3.21)
project(mouduleglfw)
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



# lib: glfw (optional)
set(LIB_GLFW_ENABLED true CACHE BOOL "Use glfw library?")
find_package(glfw3)
if(glfw3_FOUND)
else()
	set(LIB_GLFW_ENABLED false CACHE BOOL "glfw library not found" FORCE)
endif()
if(${LIB_GLFW_ENABLED})
	list(APPEND DEPS glfw)
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_GLFW=1)
else()
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_GLFW=0)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND DEPS Threads::Threads)


#=======================================================================================
#    target moduleglfw
#=======================================================================================

add_library(moduleglfw SHARED
	src/lib/base/array.cpp
	src/lib/base/pointer.cpp
	src/lib/base/strings.cpp
	src/main.cpp)
target_include_directories(moduleglfw PUBLIC ${INCLUDE_DIRECTORIES})
target_link_directories(moduleglfw PUBLIC ${LINK_DIRECTORIES})
target_compile_options(moduleglfw PUBLIC ${COMPILE_OPTIONS})
target_compile_definitions(moduleglfw PUBLIC ${COMPILE_DEFINITIONS}  INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
target_link_libraries(moduleglfw PUBLIC ${DEPS})


#=======================================================================================
#    install
#=======================================================================================

set(RESOURCE_DESTINATION share/mouduleglfw)

# binaries	

# resources
install(FILES "glfw.kaba" DESTINATION ${RESOURCE_DESTINATION})