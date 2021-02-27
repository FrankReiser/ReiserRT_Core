# Public Header files
set( _publicHeaders
        ${CMAKE_CURRENT_LIST_DIR}/RingBufferSimple.hpp
        ${CMAKE_CURRENT_LIST_DIR}/Semaphore.hpp
        ${CMAKE_CURRENT_LIST_DIR}/RingBufferGuarded.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ObjectPool.hpp
        ${CMAKE_CURRENT_LIST_DIR}/ObjectQueue.hpp
        ${CMAKE_CURRENT_LIST_DIR}/MessageQueue.hpp
        )

# Private Header files (none until otherwise determined)
set( _privateHeaders
        ""
        )

# Private Source files.
set( _privateSources
        ${CMAKE_CURRENT_LIST_DIR}/RingBufferSimple.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Semaphore.cpp
        ${CMAKE_CURRENT_LIST_DIR}/RingBufferGuarded.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ObjectPool.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ObjectQueue.cpp
        ${CMAKE_CURRENT_LIST_DIR}/MessageQueue.cpp
        )


# If we are under UNIX/Linux the pthreads library should be available and we are dependent on
# requiring the mutex attribute operations for priority inheritance control.
# Otherwise, we do not do much with pthreads here. It looks good,
# but the cache has "PTHREAD_LOC," variable in it. That's right, it includes a comma unlike
# any other variable in the cache. Seems to work okay though and there just might be a reason for it.
if(UNIX)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREAD_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    set( _REISER_RT_HAS_PTHREADS ON)

endif()

if(_REISER_RT_HAS_PTHREADS)
    list( APPEND _privateHeaders
            ${CMAKE_CURRENT_LIST_DIR}/PriorityInheritMutex.hpp
            )
    list( APPEND _privateSources
            ${CMAKE_CURRENT_LIST_DIR}/PriorityInheritMutex.cpp
            )
endif()

# Now the entire Library, currently SHARED by default. Could make an option though.
#note Testing of RingBufferGuarded fails when SHARED and using ctest. I passes when run without ctest or when STATIC. Why?
#Okay, apparently, ctest was somehow timing out on something but I am not sure what because this test was shorter
#than the messageQueue test. Perhaps it was more intense in some other way. Anyway, reducing my test queue size
#remedied the problem, so it is still unknown as to why this occurred.
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
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}
            PUBLIC_HEADER ${CMAKE_CURRENT_LIST_DIR}/RingBufferSimple.hpp
#            PUBLIC_HEADER ${CMAKE_CURRENT_LIST_DIR}/Semaphore.hpp
        )

target_compile_definitions(ReiserRT_Core
        PRIVATE
        $<$<BOOL:${_REISER_RT_HAS_PTHREADS}>:REISER_RT_HAS_PTHREADS>
        )

if(_REISER_RT_HAS_PTHREADS)
    target_link_libraries(ReiserRT_Core Threads::Threads)
endif()


# Specify our target interfaces for external clients
target_include_directories( ReiserRT_Core
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}
        )

# We do not actually link Threads::Threads at this time but, it creates a requirement that will eventually have to be satisfied.
# Anything that links to 'Us', needs Threads::Threads also.
target_link_libraries(ReiserRT_Core Threads::Threads)

# Prepare RPATH
#[[
file(RELATIVE_PATH _rel ${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR}
        ${CMAKE_INSTALL_PREFIX})
if(APPLE)
    set(_rpath "@loader_path/${_rel}")
else()
    set(_rpath "\$ORIGIN/${_rel}")
endif()
file(TO_NATIVE_PATH "${_rpath}/${INSTALL_LIBDIR}" message_RPATH)
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
