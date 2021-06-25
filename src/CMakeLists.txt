# Public Header files
set( _publicHeaders
        ReiserRT_CoreExceptions.hpp
        RingBufferSimple.hpp
        Mutex.hpp
        Semaphore.hpp
        RingBufferGuarded.hpp
        ObjectPoolBase.hpp
        ObjectPool.hpp
        ObjectPoolDeleter.hpp
        ObjectPoolFwd.hpp
        MessageQueueBase.hpp
        MessageQueue.hpp
        )

# Private Header files (none until otherwise determined)
set( _privateHeaders
        ""
        )

# Private Source files.
set( _privateSources
        ReiserRT_CoreExceptions.cpp
        RingBufferSimple.cpp
        Mutex.cpp
        Semaphore.cpp
        RingBufferGuarded.cpp
        ObjectPoolBase.cpp
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

#[[
# These are no longer private. However, I'm leaving this snippet in place as an example of how to append
# to an existing list.
if(_REISER_RT_HAS_PTHREADS)
    list( APPEND _privateHeaders ${CMAKE_CURRENT_LIST_DIR}/Mutex.hpp)
    list( APPEND _privateSources ${CMAKE_CURRENT_LIST_DIR}/Mutex.cpp)
endif()
]]

#[[
# Just an experiment
find_package(PkgConfig QUIET)
message( STATUS "PKG_CONFIG_FOUND: ${CMAKE_USE_PTHREADS_INIT}" )
]]

# Now the entire Library, currently SHARED by default. Could make an option though.
add_library( ReiserRT_Core SHARED "" )
target_sources( ReiserRT_Core
        PRIVATE
            ${_privateSources}
#            ${_privateHeaders}
#        PUBLIC
#            ${_publicHeaders}
        )

set_target_properties(ReiserRT_Core
        PROPERTIES
            POSITION_INDEPENDENT_CODE 1
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN 1
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
            DEBUG_POSTFIX "_d"
            PUBLIC_HEADER "${_publicHeaders};${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/ReiserRT_CoreExport.h"
        )

target_compile_definitions(ReiserRT_Core
        PUBLIC
            $<$<BOOL:${_REISER_RT_HAS_PTHREADS}>:REISER_RT_HAS_PTHREADS>
# I have no use for this feature at this time, so I am not going to incorporate it.
#        INTERFACE
#            $<INSTALL_INTERFACE:USING_ReiserRT_Core>
        )

# We do not actually link Threads::Threads at this time but, this creates a requirement that will eventually have to be satisfied.
# Anything that links to 'Us', needs Threads::Threads also.
# Rationale: Our tests will need Threads to do adequate testing. Any client would most certainly be using threads also.
# If Mutex amd Semaphore compile under REISER_RT_HAS_PTHREADS then Threads::Threads
# would be almost guaranteed to be needed by clients as most of ReiserRT_Core uses Mutex and/or Semaphore.
target_link_libraries(ReiserRT_Core PUBLIC Threads::Threads)

# Specify our target interfaces for external clients
target_include_directories( ReiserRT_Core
        PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}>"
            "$<INSTALL_INTERFACE:${INSTALL_INCLUDEDIR}>"
        )

include(GenerateExportHeader)
generate_export_header(ReiserRT_Core
        BASE_NAME "ReiserRT_Core"
        EXPORT_MACRO_NAME "ReiserRT_Core_EXPORT"
        EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/ReiserRT_CoreExport.h"
        DEPRECATED_MACRO_NAME "ReiserRT_Core_DEPRECATED"
        NO_EXPORT_MACRO_NAME "ReiserRT_Core_NO_EXPORT"
        STATIC_DEFINE "ReiserRT_Core_STATIC_DEFINE"
        NO_DEPRECATED_MACRO_NAME "ReiserRT_Core_NO_DEPRECATED"
        DEFINE_NO_DEPRECATED
        )

install(
    TARGETS
        ReiserRT_Core
    EXPORT
        ReiserRT_CoreTargets
    ARCHIVE
        DESTINATION ${INSTALL_LIBDIR}
        COMPONENT lib
    RUNTIME
        DESTINATION ${INSTALL_BINDIR}
        COMPONENT bin
    LIBRARY
        DESTINATION ${INSTALL_LIBDIR}
        COMPONENT lib
    PUBLIC_HEADER
        DESTINATION "${INSTALL_INCLUDEDIR}"
        COMPONENT dev
)

install (
        EXPORT
            ReiserRT_CoreTargets
        NAMESPACE
            "ReiserRT_Core::"
        DESTINATION
            ${INSTALL_CMAKEDIR}
        COMPONENT
            dev
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/ReiserRT_CoreConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
        ${PROJECT_SOURCE_DIR}/cmake/ReiserRT_CoreConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ReiserRT_CoreConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CMAKEDIR}
)

install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/ReiserRT_CoreConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ReiserRT_CoreConfigVersion.cmake
        DESTINATION
            ${INSTALL_CMAKEDIR}
)
