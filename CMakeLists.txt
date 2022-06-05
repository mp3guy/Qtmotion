cmake_minimum_required(VERSION 3.10)

project(Qtmotion)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)

find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_qtc_plugin(Qtmotion
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::TextEditor
  DEPENDS
    Qt6::Widgets
  SOURCES
    Plugin.cpp
    Plugin.h
    TargetString.cpp
    TargetString.h
    EventHandler.cpp
    EventHandler.h
)

target_compile_options(Qtmotion PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4>)
