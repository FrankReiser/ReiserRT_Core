add_executable(testRingBufferSimple "" )
target_sources( testRingBufferSimple PRIVATE testRingBufferSimple.cpp )
target_link_libraries(testRingBufferSimple ReiserRT )

add_test(
        NAME
        runRingBufferSimpleTest
        COMMAND
        $<TARGET_FILE:testRingBufferSimple> )
