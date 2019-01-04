cmake_minimum_required(VERSION 2.8)

project(MyBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -oFast ")
include_directories(${CMAKE_SOURCE_DIR}/hlt)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)

foreach(dir ${dirs})
    file(GLOB_RECURSE SOURCE ${dir}/*.[ch]*)
    set(SOURCE_FILES "${SOURCE_FILES}" ${SOURCE})
endforeach()

include_directories(${CMAKE_SOURCE_DIR})
set(SOURCE_FILES1 "${SOURCE_FILES}" MyBot.cpp)
set(SOURCE_FILES2 "${SOURCE_FILES}" ReferenceBot.cpp)

add_executable(MyBot ${SOURCE_FILES1})
add_executable(ReferenceBot ${SOURCE_FILES2})

if(MINGW)
    target_link_libraries(MyBot -static)
endif()
