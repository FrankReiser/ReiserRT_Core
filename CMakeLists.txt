cmake_minimum_required(VERSION 3.17)
#Google this: linux shared library cmake example

project(
        ReiserRT
        VERSION 1.0.1
        DESCRIPTION "Frank Reiser's Realtime Core Library")

set(CMAKE_CXX_STANDARD 14)

set(DEFAULT_BUILD_TYPE "Release")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

include(GNUInstallDirs)

include_directories(
        ReiserRT)

#add_executable(ReiserRT
#        ReiserRT/PlatformDetect.hpp
#        ReiserRT/PriorityInheritMutex.cpp
#        ReiserRT/PriorityInheritMutex.hpp
#        ReiserRT/Semaphore.cpp
#        ReiserRT/Semaphore.hpp)

add_library(
        ReiserRT SHARED
        ReiserRT/PlatformDetect.hpp
        ReiserRT/PriorityInheritMutex.cpp
        ReiserRT/PriorityInheritMutex.hpp
        ReiserRT/Semaphore.cpp
        ReiserRT/Semaphore.hpp
        ReiserRT/RingBufferSimple.hpp
        ReiserRT/RingBufferGuarded.hpp
        ReiserRT/ObjectPool.hpp
        ReiserRT/ObjectQueue.cpp
        ReiserRT/ObjectQueue.hpp
        ReiserRT/MessageQueue.hpp)

set_target_properties(ReiserRT PROPERTIES VERSION ${PROJECT_VERSION})

set_target_properties(ReiserRT PROPERTIES SOVERSION 1)

set_target_properties(ReiserRT PROPERTIES PUBLIC_HEADER ReiserRT/PlatformDetect.hpp)
set_target_properties(ReiserRT PROPERTIES PUBLIC_HEADER ReiserRT/PriorityInheritMutex.hpp)
set_target_properties(ReiserRT PROPERTIES PUBLIC_HEADER ReiserRT/Semaphore.hpp)
set_target_properties(ReiserRT PROPERTIES PUBLIC_HEADER ReiserRT/RingBufferSimple.hpp)
set_target_properties(ReiserRT PROPERTIES PUBLIC_HEADER ReiserRT/RingBufferGuarded.hpp)
set_target_properties(ReiserRT PROPERTIES PUBLIC_HEADER ReiserRT/ObjectPool.hpp)
set_target_properties(ReiserRT PROPERTIES PUBLIC_HEADER ReiserRT/ObjectQueue.hpp)
set_target_properties(ReiserRT PROPERTIES PUBLIC_HEADER ReiserRT/MessageQueue.hpp)
