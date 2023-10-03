# We are creating a build environment for a shared library named the same as
# our project in the top level CMakeLists.txt file.
add_library( ${PROJECT_NAME} SHARED "" )

# Specify all of our public headers for easy reference.
set( _publicHeaders
        ReiserRT_CoreExceptions.hpp
        RingBufferSimple.hpp
        Mutex.hpp
        Semaphore.hpp
        RingBufferGuarded.hpp
        MemoryPoolBase.hpp
        MemoryPoolDeleterBase.hpp
        ObjectPool.hpp
        ObjectPoolDeleter.hpp
        ObjectPoolFwd.hpp
        MessageQueueBase.hpp
        MessageQueue.hpp
        )

# Specify all of our private headers for easy reference.
set( _privateHeaders
      ""
    )

# Specify our source files
set( _sourceFiles
        ReiserRT_CoreExceptions.cpp
        RingBufferSimple.cpp
        Mutex.cpp
        Semaphore.cpp
        RingBufferGuarded.cpp
        MemoryPoolBase.cpp
        MemoryPoolDeleterBase.cpp
        ObjectPool.cpp
        ObjectPoolDeleter.cpp
        ObjectPoolFwd.cpp
        MessageQueueBase.cpp
        MessageQueue.cpp
        )

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREAD_PREFER_PTHREAD_FLAG TRUE)
#[[
include(FindThreads)
]]
find_package(Threads REQUIRED)
message( STATUS "Threads_FOUND: ${Threads_FOUND}")
message( STATUS "CMAKE_THREAD_LIBS_INIT: ${CMAKE_THREAD_LIBS_INIT}" )
message( STATUS "CMAKE_USE_WIN32_THREADS_INIT: ${CMAKE_USE_WIN32_THREADS_INIT}" )
message( STATUS "CMAKE_USE_PTHREADS_INIT: ${CMAKE_USE_PTHREADS_INIT}" )
if(CMAKE_USE_PTHREADS_INIT)
    set( _REISER_RT_HAS_PTHREADS ON)
endif()

# Now, Specify Sources to be built into our library
target_sources( ${PROJECT_NAME} PRIVATE ${_sourceFiles} )

target_compile_definitions(${PROJECT_NAME}
        PUBLIC
            $<$<BOOL:${_REISER_RT_HAS_PTHREADS}>:REISER_RT_HAS_PTHREADS>
# I have no use for this feature at this time, so I am not going to incorporate it.
#        INTERFACE
#            $<INSTALL_INTERFACE:USING_${PROJECT_NAME}>
        )

# Specify our target interfaces for ourself and external clients post installation
target_include_directories( ${PROJECT_NAME}
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}>"
        "$<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>"
        )

# We do not actually link Threads::Threads at this time but, this creates a requirement that will eventually have to be satisfied.
# Anything that links to 'Us', needs Threads::Threads also.
# Rationale: Our tests will need Threads to do adequate testing. Any client would most certainly be using threads also.
# If Mutex amd Semaphore compile under REISER_RT_HAS_PTHREADS then Threads::Threads
# would be almost guaranteed to be needed by clients as most of ReiserRT_Core uses Mutex and/or Semaphore.
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

# Specify Shared Object used Position Independent Code Major, the Major Version, Debug Prefix and Public Headers.
# NOTE: Additional properties set or overridden after Export Header generated below.
set_target_properties( ${PROJECT_NAME}
        PROPERTIES
        POSITION_INDEPENDENT_CODE 1
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        DEBUG_POSTFIX "_d"
        PUBLIC_HEADER "${_publicHeaders}"
)

target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

# Generate Export Header File
include(GenerateExportHeader)
generate_export_header( ${PROJECT_NAME}
        BASE_NAME "${PROJECT_NAME}"
        EXPORT_MACRO_NAME "${PROJECT_NAME}_EXPORT"
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/${PROJECT_NAME}Export.h"
        DEPRECATED_MACRO_NAME "${PROJECT_NAME}_DEPRECATED"
        NO_EXPORT_MACRO_NAME "${PROJECT_NAME}_NO_EXPORT"
        STATIC_DEFINE "${PROJECT_NAME}_STATIC_DEFINE"
        NO_DEPRECATED_MACRO_NAME "${PROJECT_NAME}_NO_DEPRECATED"
        DEFINE_NO_DEPRECATED
        )

# Since we have an EXPORT file, hide everything that isn't explicitly exported.
# Additionally, rewrite public headers appending the Export File. All of these includes need
# be part of the installation.
get_target_property( _tmp ${PROJECT_NAME} PUBLIC_HEADER )
set_target_properties( ${PROJECT_NAME}
        PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
        PUBLIC_HEADER "${_tmp};${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/${PROJECT_NAME}Export.h"
        )

# Process CMake configuration input file which dynamically generates the output CMake configuration files.
# This aides the integration with other CMake client projects that use this project.
# The first part specifies version compatibility for clients and the the second part generates the CMake
# configuration for the development purposes.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
        ${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION ${INSTALL_CMAKEDIR}
)

# Installation of Versioned Shared object library and CMake configuration files.
install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${INSTALL_LIBDIR} COMPONENT lib
        LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT lib
        RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT bin
        PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDEDIR}" COMPONENT dev
)

install(
        EXPORT ${PROJECT_NAME}Targets
        NAMESPACE "${PROJECT_NAME}::"
        DESTINATION ${INSTALL_CMAKEDIR}
        COMPONENT dev
)

install(
        FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${INSTALL_CMAKEDIR}
)
