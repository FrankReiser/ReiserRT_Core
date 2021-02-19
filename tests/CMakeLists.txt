add_executable( testRingBufferSimple "" )
target_sources( testRingBufferSimple PRIVATE testRingBufferSimple.cpp )
target_link_libraries( testRingBufferSimple ReiserRT )
add_test( NAME runRingBufferSimpleTest COMMAND $<TARGET_FILE:testRingBufferSimple> )

# We do not have a test for PriorityInheritMutex. It is tested extensively inside our Semaphore test
# assuming that it was conditionally compiled into the library. It's only available under certain platforms.
add_executable( testSemaphore "" )
target_sources( testSemaphore PRIVATE testSemaphore.cpp semTakeTasks.cpp semTakeTasks.h)
target_link_libraries( testSemaphore ReiserRT )
add_test( NAME runSemaphoreTest COMMAND $<TARGET_FILE:testSemaphore> )
