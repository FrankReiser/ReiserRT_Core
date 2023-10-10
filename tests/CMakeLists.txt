add_executable( testRingBufferSimple "" )
target_sources( testRingBufferSimple PRIVATE testRingBufferSimple.cpp )
target_include_directories( testRingBufferSimple PUBLIC ../src )
target_link_libraries( testRingBufferSimple ReiserRT_Core )
target_compile_options( testRingBufferSimple PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
add_test( NAME runRingBufferSimpleTest COMMAND $<TARGET_FILE:testRingBufferSimple> )

# We will use a common object library for our StartingGun
#add_library( startingGunObjLib OBJECT StartingGun.h StartingGun.cpp )
#target_sources( startingGunObjLib PUBLIC StartingGun.h PRIVATE StartingGun.cpp )

# We do not have a test for CoreMutex. It is tested extensively inside our Semaphore test
# assuming that it was conditionally compiled into the library. It's only available under certain platforms.

add_executable( testSemaphore "" )
target_sources( testSemaphore PRIVATE
        testSemaphore.cpp
        SemTestTasks.h SemTestTasks.cpp
        StartingGun.h StartingGun.cpp )
target_include_directories( testSemaphore PUBLIC ../src )
target_link_libraries( testSemaphore ReiserRT_Core )
target_compile_options( testSemaphore PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
add_test( NAME runSemaphoreTest COMMAND $<TARGET_FILE:testSemaphore> )
set_tests_properties( runSemaphoreTest PROPERTIES TIMEOUT 120 )

add_executable( testRingBufferGuarded "" )
target_sources( testRingBufferGuarded PRIVATE
        testRingBufferGuarded.cpp
        RingBufferGuardedTestTasks.h RingBufferGuardedTestTasks.cpp
        StartingGun.h StartingGun.cpp )
target_include_directories( testRingBufferGuarded PUBLIC ../src )
target_link_libraries( testRingBufferGuarded ReiserRT_Core )
target_compile_options( testRingBufferGuarded PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
add_test( NAME runRingBufferGuardedTest COMMAND $<TARGET_FILE:testRingBufferGuarded> )
set_tests_properties( runRingBufferGuardedTest PROPERTIES TIMEOUT 120 )

add_executable( testObjectPool "" )
target_sources( testObjectPool PRIVATE testObjectPool.cpp )
target_include_directories( testObjectPool PUBLIC ../src )
target_link_libraries( testObjectPool ReiserRT_Core )
target_compile_options( testObjectPool PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
add_test( NAME runObjectPoolTest COMMAND $<TARGET_FILE:testObjectPool> )

add_executable( testMessageQueue "" )
target_sources( testMessageQueue PRIVATE testMessageQueue.cpp )
target_include_directories( testMessageQueue PUBLIC ../src )
target_link_libraries( testMessageQueue ReiserRT_Core )
target_compile_options( testMessageQueue PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
add_test( NAME runMessageQueueTest COMMAND $<TARGET_FILE:testMessageQueue> )
set_tests_properties( runMessageQueueTest PROPERTIES TIMEOUT 120 )

add_executable( testBlockPool "" )
target_sources( testBlockPool PRIVATE testBlockPool.cpp )
target_include_directories( testBlockPool PUBLIC ../src )
target_link_libraries( testBlockPool ReiserRT_Core )
target_compile_options( testBlockPool PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)
add_test( NAME runBlockPoolTest COMMAND $<TARGET_FILE:testBlockPool> )
