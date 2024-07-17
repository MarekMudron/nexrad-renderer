cmake_minimum_required(VERSION 3.12.0)

set(CMAKE_CXX_VISIBILITY_PRESET default)

PROJECT(FitGraphics)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_C_STANDARD 99)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1 ) 

option(CLEAR_CMAKE_ROOT_DIR "if this is set, #define CMAKE_ROOT_DIR will be .")


set(LIBS_SOURCES
  libs/tiny_gltf/tiny_gltf.h
  libs/stb_image/stb_image.h
  libs/stb_image/stb_image_write.h
  libs/json/json.hpp
  libs/impl.cpp
  )

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE libs/glm-0.9.9.8)

add_subdirectory(libs/MealyMachine)
add_subdirectory(libs/TxtUtils)
add_subdirectory(libs/ArgumentViewer)
add_subdirectory(libs/BasicCamera)

add_subdirectory(libs/SDL2CPP)
add_subdirectory(libs/geGL)
add_subdirectory(libs/Vars)
add_subdirectory(libs/imguiDormon)
add_subdirectory(libs/imguiOpenGLDormon)
add_subdirectory(libs/imguiSDL2Dormon)
add_subdirectory(libs/imguiSDL2OpenGL)
add_subdirectory(libs/imguiVars)
add_subdirectory(libs/Simple3DApp)
add_subdirectory(libs/VarsGLMDecorator)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

option(SDL_SHARED "" OFF)
option(SDL_STATIC "" ON)
add_subdirectory(libs/SDL-release-2.24.1)
#add_subdirectory(libs/SDL2-2.0.14)
#add_library(SDL2::SDL2 ALIAS SDL2-static)
#add_library(SDL2::SDL2main ALIAS SDL2main)


include(CMakeUtils.cmake)

set(FRAMEWORK_SOURCES 
  libs/impl.cpp
  src/framework/DVars.hpp
  src/framework/Barrier.cpp
  src/framework/Barrier.h
  src/framework/FunctionPrologue.h
  src/framework/methodRegister.hpp
  src/framework/methodRegister.cpp
  src/framework/Timer.hpp
  src/framework/model.cpp
  src/framework/model.hpp
  src/framework/bunny.cpp
  src/framework/bunny.hpp
  src/framework/OpenGLState.cpp
  src/framework/OpenGLState.hpp
  src/framework/methodManager.hpp
  src/framework/methodManager.cpp
  src/framework/defineGLSLVersion.cpp
  src/framework/defineGLSLVersion.hpp
  src/framework/makeProgram.cpp
  src/framework/makeProgram.hpp
  )

add_library(framework ${FRAMEWORK_SOURCES})
target_include_directories(framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/json)
target_include_directories(framework PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/stb_image)
target_link_libraries(framework
  Simple3DApp::Simple3DApp 
  Vars::Vars 
  VarsGLMDecorator::VarsGLMDecorator 
  imguiDormon::imguiDormon 
  imguiVars::imguiVars
  BasicCamera::BasicCamera
  )
if(NOT CLEAR_CMAKE_ROOT_DIR)
  target_compile_definitions(framework PUBLIC CMAKE_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
else()
  target_compile_definitions(framework PUBLIC CMAKE_ROOT_DIR=".")
endif()


set(PGR01_SOURCES
  src/PGR/01/emptyWindow.hpp
  src/PGR/01/emptyWindow.cpp
  src/PGR/01/oglFunc.cpp
  src/PGR/01/compileShaders.cpp
  src/PGR/01/compileShaders.hpp
  src/PGR/01/shaders.cpp
  src/PGR/01/shaders.hpp
  src/PGR/01/primitives.hpp
  src/PGR/01/primitives.cpp
  src/PGR/01/buffers.hpp
  src/PGR/01/buffers.cpp
  src/PGR/01/vertexArrays.hpp
  src/PGR/01/vertexArrays.cpp
  src/PGR/01/camera.hpp
  src/PGR/01/camera.cpp
  src/PGR/01/task.hpp
  src/PGR/01/task.cpp
  src/PGR/01/freelookCamera.hpp
  src/PGR/01/freelookCamera.cpp
  src/PGR/01/geometryShaders.hpp
  src/PGR/01/geometryShaders.cpp
  )

set(PGR02_SOURCES
  src/PGR/02/uv.hpp
  src/PGR/02/uv.cpp
  src/PGR/02/texture.hpp
  src/PGR/02/texture.cpp
  src/PGR/02/model.hpp
  src/PGR/02/model.cpp
  )

set(PGR03_SOURCES
  src/PGR/03/phong.hpp
  src/PGR/03/phong.cpp
  src/PGR/03/framebuffer.hpp
  src/PGR/03/framebuffer.cpp
  src/PGR/03/shadowMapping.hpp
  src/PGR/03/shadowMapping.cpp
  src/PGR/03/shadowedModel.hpp
  src/PGR/03/shadowedModel.cpp
  src/PGR/03/twoTriangles.hpp
  src/PGR/03/twoTriangles.cpp
  src/PGR/03/bezierSurf.cpp
  src/PGR/03/stencil.cpp
  src/PGR/03/shadowVolume.cpp
  )

set(PGR_EXAMPLE_SOURCES
  ${PGR01_SOURCES}
  ${PGR02_SOURCES}
  ${PGR03_SOURCES}
  )

add_library(PGR OBJECT ${PGR_EXAMPLE_SOURCES})
target_link_libraries(PGR framework)

set(GMU_EXAMPLE_SOURCES
  src/GMU/lab/nbody.hpp
  src/GMU/lab/nbody.cpp
  )

add_library(GMU OBJECT ${GMU_EXAMPLE_SOURCES})
target_link_libraries(GMU framework)

set(PGP_SOURCES
  src/PGP/lect/compute.hpp
  src/PGP/lect/compute.cpp
  src/PGP/homework1.cpp
  src/PGP/homework2.cpp
  src/PGP/homework3.cpp
  src/PGP/homework4.cpp
  src/PGP/homework5.cpp
  )

add_library(PGP OBJECT ${PGP_SOURCES})
target_link_libraries(PGP framework)

set(LKG_SOURCES
  src/LKG/lkgp.hpp
  src/LKG/lkgp.cpp
  src/LKG/lf_blend.hpp
  src/LKG/lf_blend.cpp
  src/LKG/spherical_harmonic_visualizer.cpp
  src/LKG/spherical_harmonic_visualizer.hpp
  )

add_library(LKG OBJECT ${LKG_SOURCES})
target_link_libraries(LKG framework)

set(STUDENT_SOURCES
  src/student/project.cpp
  )

##############################################
include_directories(
  /usr/local/lrose/include
)
link_directories(
    /usr/lib/x86_64-linux-gnu/hdf5/serial
    /usr/local/lrose/lib
)
##############################################

add_library(STUDENT OBJECT ${STUDENT_SOURCES})
target_link_libraries(STUDENT 
framework
Radx 
Ncxx 
hdf5 
netcdf 
bz2 
toolsa
dataport
)


add_subdirectory(src/MISC)

set(SOLUTION_DIR "" CACHE STRING "Directory containing solution files")

if(NOT ${SOLUTION_DIR} EQUAL "")
  add_subdirectory(${SOLUTION_DIR} SOLUTION)
endif()


set(APP_SOURCES
  src/framework/main.cpp
  )

add_executable(${PROJECT_NAME} 
  ${APP_SOURCES} 
  )

set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS 1)
target_link_libraries(${PROJECT_NAME}
  framework
  PGR
  PGP
  GMU
  ${MISC_LIBRARY}
  LKG
  STUDENT
  ${SOLUTION_LIBRARY}
  )

if (CMAKE_CROSSCOMPILING)
  target_link_libraries(${PROJECT_NAME}  
    -lmingw32
    -static-libgcc 
    -static-libstdc++
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/json)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs/stb_image)

add_custom_target(run ./${PROJECT_NAME})


source_group("framework" FILES ${FRAMEWORK_SOURCES})
source_group("app"       FILES ${APP_SOURCES})
source_group("pgr01"     FILES ${PGR01_SOURCES})
source_group("pgr02"     FILES ${PGR02_SOURCES})
source_group("pgr03"     FILES ${PGR03_SOURCES})
source_group("lkg"       FILES ${LKG_SOURCES})
source_group("pgp"       FILES ${PGP_SOURCES})
source_group("gpu"       FILES ${GPU_SOURCES})
source_group("student"   FILES ${STUDENT_SOURCES})

