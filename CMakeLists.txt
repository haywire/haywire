cmake_minimum_required(VERSION 2.6)
include("common.cmake")

# ----------------------------------------
# Haywire
# ----------------------------------------
project(haywire C)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
#set(CMAKE_BUILD_TYPE Debug)

add_definitions(-std=gnu99)
#add_definitions(-mavx)
add_definitions(-msse4.1)
add_definitions(-pedantic)
add_definitions(-O3)
add_definitions(-Wall)
add_definitions(-Wextra)
add_definitions(-Wcast-align)
add_definitions(-w)

if (UNIX)
    add_definitions(-DUNIX)
endif (UNIX)

file(GLOB_RECURSE HAYWIRE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/haywire/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/haywire/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/haywire/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/haywire/*.cpp)

list(SORT HAYWIRE_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}/src" ${HAYWIRE_SOURCES})
include_directories(${CMAKE_SOURCE_DIR}/lib/libuv/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

add_library(haywire STATIC ${HAYWIRE_SOURCES})

target_link_libraries (haywire ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_SOURCE_DIR}/lib/libuv/.libs/libuv.a)

GET_PROPERTY(haywire_location TARGET haywire PROPERTY LOCATION)

# ----------------------------------------
# Hello world sample
# ----------------------------------------
file(GLOB_RECURSE HELLOWORLD_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/samples/hello_world/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/samples/hello_world/*.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/samples/hello_world/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/samples/hello_world/*.cpp)

list(SORT HELLOWORLD_SOURCES)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}/src" ${HELLOWORLD_SOURCES})
include_directories(${CMAKE_SOURCE_DIR}/lib/libuv/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

add_executable (hello_world
    ${HELLOWORLD_SOURCES})

add_dependencies(hello_world haywire)

# Libraries to link in reverse order because that's what ld requires.
target_link_libraries (hello_world
    ${haywire_location}
    ${CMAKE_SOURCE_DIR}/lib/libuv/.libs/libuv.a
    ${CMAKE_THREAD_LIBS_INIT})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries (hello_world rt)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
