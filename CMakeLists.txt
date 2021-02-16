cmake_minimum_required(VERSION 3.17)
#Google this: linux shared library cmake example

project(
        ReiserRT
        VERSION 1.0.1
        DESCRIPTION "Frank Reiser's Realtime Core Library")

set(CMAKE_CXX_STANDARD 11)

# This will show its output only in ccmake. However, you have to do a "configure" (c) before
# you can view the log (l). PROJECT_NAME is ReiserRT
#message( STATUS "ProjectName: ${PROJECT_NAME} ***********************************" )

# Can't remember Why I tried this
#set(DEFAULT_BUILD_TYPE "Release")

# Can't remember Why I tried this
#if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
#    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
#    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
#    # Set the possible values of build type for cmake-gui
#    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
#endif()

# Can't remember Why I tried this
#include(GNUInstallDirs)

# Proving UNIX is defined on Linux
#message( STATUS "PROJECT_VERSION_PATCH: ${PROJECT_VERSION_PATCH} ***********************************" )
#if(UNIX)
#message( STATUS "UNIX: Defined ***********************************" )
#endif(UNIX)

# Our include directories are the same as our project directory
# plus we have a ProjectConfigure header file in the build directory.
include_directories(${PROJECT_NAME} ${PROJECT_BINARY_DIR} )
#message( STATUS "PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR} ***********************************" )

# If we are under UNIX/Linux the pthreads library should be available and we are dependent on
# requiring the mutex attribute operations for priority inheritance control.
# Otherwise, we do not do much with pthreads here. It looks good,
# but the cache has "PTHREAD_LOC," variable in it. That's right, it includes a comma unlike
# any other variable in the cache. Seems to work okay though and there just might be a reason for it.
if(UNIX)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREAD_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    # Assume this is pthreads for purposes of "Configuration"
    set(REISER_RT_HAS_PTHREADS ON)
endif(UNIX)

configure_file(
        ${PROJECT_SOURCE_DIR}/ProjectConfigure.h.in
        ${PROJECT_BINARY_DIR}/ProjectConfigure.h )

# We are building a "Shared" library
add_library(
        ${PROJECT_NAME} SHARED
        ${PROJECT_NAME}/PriorityInheritMutex.cpp
        ${PROJECT_NAME}/Semaphore.cpp
        ${PROJECT_NAME}/ObjectQueue.cpp
)

# Library Version and SOVERSION
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_PATCH})

# Private Header Files
set_target_properties(${PROJECT_NAME} PROPERTIES PRIVATE_HEADER ${PROJECT_BINARY_DIR}/ProjectConfigure.h)

# Public Header files
#@todo This only gets the last one, "MessageQueue.hpp. Trying to concatenate them all into one
#set property call gets just the first one.
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${PROJECT_NAME}/PriorityInheritMutex.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${PROJECT_NAME}/Semaphore.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${PROJECT_NAME}/RingBufferSimple.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${PROJECT_NAME}/RingBufferGuarded.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${PROJECT_NAME}/ObjectPool.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${PROJECT_NAME}/ObjectQueue.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER ${PROJECT_NAME}/MessageQueue.hpp)

if(UNIX)
    target_link_libraries(${PROJECT_NAME} Threads::Threads)
endif(UNIX)


# This will install just the lib currently (1 file and 2 links) to the lib directory.
# The test program will probably get installed here too when I get that far.
# NOTE: THIS TARGET REQUIRES root privileges to complete.
install(
        TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/${PROJECT_NAME}     #???
        )

install( FILES
        ${PROJECT_NAME}/PriorityInheritMutex.hpp
        ${PROJECT_NAME}/Semaphore.hpp
        ${PROJECT_NAME}/RingBufferSimple.hpp
        ${PROJECT_NAME}/RingBufferGuarded.hpp
        ${PROJECT_NAME}/ObjectPool.hpp
        ${PROJECT_NAME}/ObjectQueue.hpp
        ${PROJECT_NAME}/MessageQueue.hpp
        DESTINATION include/${PROJECT_NAME} )

# If Windows, I want 64bit compiler, not 32bit. That could be an option perhaps. This requires research.

