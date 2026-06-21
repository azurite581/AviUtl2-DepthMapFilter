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

# OpenCV 5.x (コミットハッシュ：6d2cd14bb21698e0582a4f8724986a3e63b54fa2) の CMakeLists.txt の不具合？により、
# GitHub Actions の Windows 環境で OpenCV をソースビルドした場合、各ターゲットのリンク依存に $<LINK_ONLY:pthread> が混入する。
# しかし Windows 環境には pthread.lib が存在しないため、LINK エラーになる。
# 一時的な回避策として、あらかじめ該当プロパティから pthread を取り除いておく。
foreach(_ocv_target opencv_core opencv_flann opencv_geometry opencv_imgproc opencv_dnn opencv_imgcodecs)
  if(TARGET ${_ocv_target})
    foreach(_prop INTERFACE_LINK_LIBRARIES IMPORTED_LINK_INTERFACE_LIBRARIES IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE)
      get_target_property(_libs ${_ocv_target} ${_prop})
      if(_libs)
        list(REMOVE_ITEM _libs "\$<LINK_ONLY:pthread>")
        list(REMOVE_ITEM _libs "pthread")
        set_target_properties(${_ocv_target} PROPERTIES ${_prop} "${_libs}")
      endif()
    endforeach()
  endif()
endforeach()

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
