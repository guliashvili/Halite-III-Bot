cmake_minimum_required(VERSION 2.8)

project(MyBot)
add_custom_command ( OUTPUT MyBot
                     COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/installOnHost.sh
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)


add_custom_target(
  BuildDocs ALL
  DEPENDS MyBot
)
