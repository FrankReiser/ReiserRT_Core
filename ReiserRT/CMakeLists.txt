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

# The PriorityInheritMutex Object Library.
add_library( priorityInheritMutexObj OBJECT "" )
target_sources( priorityInheritMutexObj
        PRIVATE
            PriorityInheritMutex.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/PriorityInheritMutex.hpp
        )
if(_REISER_RT_HAS_PTHREADS)
target_compile_definitions( priorityInheritMutexObj PRIVATE REISER_RT_HAS_PTHREADS )
endif()

# The Semaphore Object Library.
add_library( semaphoreObj OBJECT "" )
target_sources( semaphoreObj
        PRIVATE
            Semaphore.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/Semaphore.hpp
        )
if(_REISER_RT_HAS_PTHREADS)
    target_compile_definitions( semaphoreObj PRIVATE REISER_RT_HAS_PTHREADS )
endif()

# The RingBufferGuarded Object Library
add_library( ringBufferGuardedObj OBJECT "" )
target_sources( ringBufferGuardedObj
        PRIVATE
            RingBufferGuarded.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/RingBufferGuarded.hpp
        )

# The Object Object Library
add_library( ObjectPoolObj OBJECT "" )
target_sources( ObjectPoolObj
        PRIVATE
            ObjectPool.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/ObjectPool.hpp
        )

# The Object Queue Library
add_library( ObjectQueueObj OBJECT "" )
target_sources( ObjectQueueObj
        PRIVATE
            ObjectQueue.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/ObjectQueue.hpp
        )

# The Message Queue Library
add_library( MessageQueueObj OBJECT "" )
target_sources( MessageQueueObj
        PRIVATE
            MessageQueue.cpp
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/MessageQueue.hpp
        )

# Now the entire Library, currently default type for STATIC/SHARED.
add_library( ReiserRT
        $<TARGET_OBJECTS:ringBufferSimpleObj>
        $<TARGET_OBJECTS:priorityInheritMutexObj>
        $<TARGET_OBJECTS:semaphoreObj>
        $<TARGET_OBJECTS:ringBufferGuardedObj>
        $<TARGET_OBJECTS:ObjectPoolObj>
        )

# Specify our targets interface for external clients
target_include_directories( ReiserRT
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}
        )

# We do not actually link this at this time, but it creates a requirement that will eventually have to be satisfied.
# Anything that links to 'Us', needs Threads::Threads also.
target_link_libraries(ReiserRT Threads::Threads)
