cmake_minimum_required(VERSION 3.12.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1 ) 

PROJECT(misc)

set(MISC_LIBRARY ${PROJECT_NAME} PARENT_SCOPE)
set(MISC_SOURCES
  rasterizationOrder.cpp
  tessellationLevels.cpp
  memoryBandwidth.cpp
  )

add_library(${PROJECT_NAME} OBJECT ${MISC_SOURCES})
target_link_libraries(${PROJECT_NAME} framework)
