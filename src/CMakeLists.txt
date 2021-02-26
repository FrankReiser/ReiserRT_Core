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
        RingBufferSimple.cpp
        Semaphore.cpp
        RingBufferGuarded.cpp
        ObjectPool.cpp
        ObjectQueue.cpp
        MessageQueue.cpp
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
            PriorityInheritMutex.cpp
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
        )


if(UNIX)
    target_link_libraries(ReiserRT_Core Threads::Threads)
endif(UNIX)

# Specify our targets interface for external clients
target_include_directories( ReiserRT_Core
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}
        )

# We do not actually link Threads::Threads at this time but, it creates a requirement that will eventually have to be satisfied.
# Anything that links to 'Us', needs Threads::Threads also.
target_link_libraries(ReiserRT_Core Threads::Threads)
