cmake_minimum_required(VERSION 3.17)

project(
        ReiserRT_Core
        VERSION 1.0.1
        DESCRIPTION "Frank Reiser's Realtime Core Library")

# Set up compiler requirements
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type set to ${CMAKE_BUILD_TYPE}")

message(STATUS "Project will be installed to ${CMAKE_INSTALL_PREFIX}")

include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Installation directory for libraries")
set(INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR} CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
    set(DEF_INSTALL_CMAKEDIR CMake)
else()
    set(DEF_INSTALL_CMAKEDIR share/cmake/${PROJECT_NAME})
endif()
set(INSTALL_CMAKEDIR ${DEF_INSTALL_CMAKEDIR} CACHE PATH "Installation directory for CMake files")

# Report to user
foreach(p LIB BIN INCLUDE CMAKE)
    file(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/${INSTALL_${p}DIR} _path )
    message(STATUS "Installing ${p} components to ${_path}")
    unset(_path)
endforeach()


add_subdirectory( src )
enable_testing()
add_subdirectory( tests )


#[[

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

#Well this may be best for some projects, but I think for this one we are better off with the single define.
#In addition, we only want this under UNIX environment. Perhaps we even have to be careful about the host OS
#because Windows with minGW may still not support the attribute. I should verify Linux doesn't default to priority
#inheritance.
configure_file(
        ${PROJECT_SOURCE_DIR}/ProjectConfigure.h.in
        ${PROJECT_BINARY_DIR}/ProjectConfigure.h )

# We are building a "Shared" library
add_library(
#        ${PROJECT_NAME} SHARED
        ${PROJECT_NAME} # Allow default behaviour for SHARED vs STATIC as specified in cache variables.
        ${PROJECT_NAME}/PriorityInheritMutex.cpp
        ${PROJECT_NAME}/Semaphore.cpp
        ${PROJECT_NAME}/ObjectQueue.cpp
        ReiserRT/RingBufferSimple.cpp ReiserRT/RingBufferGuarded.cpp ReiserRT/ObjectPool.cpp ReiserRT/MessageQueue.cpp)

# Library Version and SOVERSION
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_PATCH})

# Private Header Files
set_target_properties(${PROJECT_NAME} PROPERTIES PRIVATE_HEADER ${PROJECT_BINARY_DIR}/ProjectConfigure.h)

# Public Header files
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

]]
