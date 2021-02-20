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

endif(UNIX)

# The RingBufferSimple Object Library.
add_library( ringBufferSimpleObj OBJECT "" )
target_sources( ringBufferSimpleObj
        PRIVATE
            RingBufferSimple.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/RingBufferSimple.hpp
        )
list( APPEND _objLibs $<TARGET_OBJECTS:ringBufferSimpleObj> )

# The PriorityInheritMutex Object Library.
if(_REISER_RT_HAS_PTHREADS)
    add_library( priorityInheritMutexObj OBJECT "" )
    target_sources( priorityInheritMutexObj
            PRIVATE
                PriorityInheritMutex.cpp
            PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/PriorityInheritMutex.hpp
            )
    target_compile_definitions( priorityInheritMutexObj PRIVATE REISER_RT_HAS_PTHREADS )
    list( APPEND _objLibs $<TARGET_OBJECTS:priorityInheritMutexObj> )
endif()

# The Semaphore Object Library.
add_library( semaphoreObjLib OBJECT "" )
target_sources( semaphoreObjLib
        PRIVATE
            Semaphore.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/Semaphore.hpp
        )
if(_REISER_RT_HAS_PTHREADS)
    target_compile_definitions( semaphoreObjLib PRIVATE REISER_RT_HAS_PTHREADS )
endif()
list( APPEND _objLibs $<TARGET_OBJECTS:semaphoreObjLib> )

# The RingBufferGuarded Object Library
add_library( ringBufferGuardedObjLib OBJECT "" )
target_sources( ringBufferGuardedObjLib
        PRIVATE
            RingBufferGuarded.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/RingBufferGuarded.hpp
        )
list( APPEND _objLibs $<TARGET_OBJECTS:ringBufferGuardedObjLib> )

# The Object Object Library
add_library( objectPoolObjLib OBJECT "" )
target_sources( objectPoolObjLib
        PRIVATE
            ObjectPool.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/ObjectPool.hpp
        )
list( APPEND _objLibs $<TARGET_OBJECTS:objectPoolObjLib> )

# The Object Queue Library
add_library( objectQueueObjLib OBJECT "" )
target_sources( objectQueueObjLib
        PRIVATE
            ObjectQueue.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/ObjectQueue.hpp
        )
list( APPEND _objLibs $<TARGET_OBJECTS:objectQueueObjLib> )

# The Message Queue Library
add_library( messageQueueObjLib OBJECT "" )
target_sources( messageQueueObjLib
        PRIVATE
            MessageQueue.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/MessageQueue.hpp
        )
list( APPEND _objLibs $<TARGET_OBJECTS:messageQueueObjLib> )

# Now the entire Library, currently default type for STATIC/SHARED.
add_library( ReiserRT ${_objLibs} )

# Specify our targets interface for external clients
target_include_directories( ReiserRT
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}
        )

# We do not actually link Threads::Threads at this time but, it creates a requirement that will eventually have to be satisfied.
# Anything that links to 'Us', needs Threads::Threads also.
target_link_libraries(ReiserRT Threads::Threads)
