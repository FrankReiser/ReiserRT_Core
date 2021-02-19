add_executable( testRingBufferSimple "" )
target_sources( testRingBufferSimple PRIVATE testRingBufferSimple.cpp )
target_link_libraries( testRingBufferSimple ReiserRT )
add_test( NAME runRingBufferSimpleTest COMMAND $<TARGET_FILE:testRingBufferSimple> )

# We will use a common object library for our StartingGun
add_library( startingGunObjLib OBJECT "" )
target_sources( startingGunObjLib PUBLIC StartingGun.h PRIVATE StartingGun.cpp )

# We do not have a test for PriorityInheritMutex. It is tested extensively inside our Semaphore test
# assuming that it was conditionally compiled into the library. It's only available under certain platforms.
add_executable( testSemaphore "" )
target_sources( testSemaphore PRIVATE testSemaphore.cpp SemTestTasks.cpp SemTestTasks.h )
target_link_libraries( testSemaphore ReiserRT $<TARGET_OBJECTS:startingGunObjLib> )
add_test( NAME runSemaphoreTest COMMAND $<TARGET_FILE:testSemaphore> )

add_executable( testRingBufferGuarded "" )
target_sources( testRingBufferGuarded PRIVATE testRingBufferGuarded.cpp RingBufferGuardedTestTasks.cpp RingBufferGuardedTestTasks.h)
target_link_libraries( testRingBufferGuarded ReiserRT $<TARGET_OBJECTS:startingGunObjLib> )
add_test( NAME runRingBufferGuardedTest COMMAND $<TARGET_FILE:testRingBufferGuarded> )

