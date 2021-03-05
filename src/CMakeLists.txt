# Public Header files
set( _publicHeaders
        ../ReiserRT_Core/RingBufferSimple.hpp
        ../ReiserRT_Core/Semaphore.hpp
        ../ReiserRT_Core/RingBufferGuarded.hpp
        ../ReiserRT_Core/ObjectPool.hpp
        ../ReiserRT_Core/ObjectQueue.hpp
        ../ReiserRT_Core/MessageQueue.hpp
        )

# Private Header files (none until otherwise determined)
set( _privateHeaders
        ""
        )

# Private Source files.
set( _privateSources
        RingBufferSimple.cpp
        Semaphore.cpp
        RingBufferGuarded.cpp
        ObjectPool.cpp
        ObjectQueue.cpp
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

if(_REISER_RT_HAS_PTHREADS)
    list( APPEND _privateHeaders ${CMAKE_CURRENT_LIST_DIR}/PriorityInheritMutex.hpp )
    list( APPEND _privateSources ${CMAKE_CURRENT_LIST_DIR}/PriorityInheritMutex.cpp )
endif()

# Just an experiment
find_package(PkgConfig QUIET)
message( STATUS "PKG_CONFIG_FOUND: ${CMAKE_USE_PTHREADS_INIT}" )



# Now the entire Library, currently SHARED by default. Could make an option though.
add_library( ReiserRT_Core SHARED "" )
target_sources( ReiserRT_Core
        PRIVATE
            ${_privateSources}
            ${_privateHeaders}
        PUBLIC
            ${_publicHeaders}
        )

set_target_properties(ReiserRT_Core
        PROPERTIES
            POSITION_INDEPENDENT_CODE 1
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN 1
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
            PUBLIC_HEADER "${_publicHeaders};${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/ReiserRT_CoreExport.h"
        )

target_compile_definitions(ReiserRT_Core
        PRIVATE
        $<$<BOOL:${_REISER_RT_HAS_PTHREADS}>:REISER_RT_HAS_PTHREADS>
        )

# We do not actually link Threads::Threads at this time but, this creates a requirement that will eventually have to be satisfied.
# Anything that links to 'Us', needs Threads::Threads also. This all assuming we found pthreads.
target_link_libraries(ReiserRT_Core PUBLIC $<$<BOOL:${_REISER_RT_HAS_PTHREADS}>:Threads::Threads>)

# Specify our target interfaces for external clients
target_include_directories( ReiserRT_Core
        PUBLIC
#            ..
            ../ReiserRT_Core
            ${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}
        )

include(GenerateExportHeader)
generate_export_header(ReiserRT_Core
        BASE_NAME "ReiserRT_Core"
        EXPORT_MACRO_NAME "ReiserRT_Core_EXPORT"
        EXPORT_FILE_NAME
        "${CMAKE_BINARY_DIR}/${INSTALL_INCLUDEDIR}/ReiserRT_CoreExport.h"
        DEPRECATED_MACRO_NAME "ReiserRT_Core_DEPRECATED"
        NO_EXPORT_MACRO_NAME "ReiserRT_Core_NO_EXPORT"
        STATIC_DEFINE "ReiserRT_Core_STATIC_DEFINE"
        NO_DEPRECATED_MACRO_NAME "ReiserRT_Core_NO_DEPRECATED"
        DEFINE_NO_DEPRECATED
        )

#@todo I think you might only need this for executable type targets. Consider for linkage requirements for users of ReiserRT_Core like the tests.
# Prepare RPATH
#[[
file(RELATIVE_PATH _rel ${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR}
        ${CMAKE_INSTALL_PREFIX})
if(APPLE)
    set(_rpath "@loader_path/${_rel}")
else()
    set(_rpath "\$ORIGIN/${_rel}")
endif()
file(TO_NATIVE_PATH "${_rpath}/${INSTALL_LIBDIR}" ReiserRT_Core_RPATH)
]]

install(
    TARGETS
        ReiserRT_Core
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
        DESTINATION ${INSTALL_INCLUDEDIR}/ReiserRT_Core
        COMPONENT dev
)
