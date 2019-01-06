cmake_minimum_required(VERSION 2.8)

project(MyBot)


if(MINGW)
    target_link_libraries(MyBot -static)
endif()
