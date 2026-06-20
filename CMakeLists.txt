cmake_minimum_required(VERSION 3.31)

if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()

project(DepthMapFilter LANGUAGES C CXX)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(${PROJECT_NAME} SHARED
  ${CMAKE_SOURCE_DIR}/src/main.cpp
)

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
target_link_libraries(${PROJECT_NAME} PRIVATE compiler_flags)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS NO)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".auf2")

target_link_libraries(${PROJECT_NAME} PRIVATE
  delayimp
)

# OpenCV
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/opencv_build/opencv")
find_package(OpenCV REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
  opencv_dnn
  opencv_imgcodecs
)

# AviUtl2 SDK
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/third_party/aviutl2_sdk_mirror/include/aviutl2_sdk)

if(WIN32)
  target_compile_definitions(${PROJECT_NAME} PRIVATE
  UNICODE
  _UNICODE

  $<$<CONFIG:Debug>:_DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)
endif()

# Compiler Options ------------------------------------------------------------------
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE
    /utf-8
    /W4
    /sdl

    /permissive-
    $<$<CONFIG:Debug>:/Zi>

    $<IF:$<CONFIG:Debug>,/Od,/O2>
    $<$<CONFIG:Release>:/Oi>

    $<IF:$<CONFIG:Debug>,/GL-,/GL>
    $<IF:$<CONFIG:Debug>,/Gy-,/Gy>
    /EHsc
  )
  target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:/LTCG>

    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>

    # Hybrid CRT
    $<$<CONFIG:Debug>:/NODEFAULTLIB:libucrtd.lib>
    $<$<CONFIG:Debug>:/DEFAULTLIB:ucrtd.lib>
    $<$<NOT:$<CONFIG:Debug>>:/NODEFAULTLIB:libucrt.lib>
    $<$<NOT:$<CONFIG:Debug>>:/DEFAULTLIB:ucrt.lib>

    # Delay load
    "/DELAYLOAD:onnxruntime.dll"
  )
endif()
