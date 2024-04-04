//
// Created by frank on 1/5/23.
//

#include "BlockPool.hpp"

#include <iostream>
#include <stdexcept>

using namespace ReiserRT::Core;

int testWithScalars()
{
    constexpr size_t NUM_BLOCKS = 4;
    constexpr size_t NUM_ELEMENTS = 24;

    BlockPool< double > scalarBufferPool{ NUM_BLOCKS, NUM_ELEMENTS };

    // The size should be 4, what we requested. If we requested three, we'd still get four.
    // This has been proven many times at various layers.
    auto blockPoolSize = scalarBufferPool.getSize();
    if ( NUM_BLOCKS != blockPoolSize )
    {
        std::cout << "Block Pool Size is " << blockPoolSize
            << " and should be " << NUM_BLOCKS << std::endl;
        return 1;
    }

    // The running stats size, low watermark and running count should all be 4 as
    // we have not asked for any blocks yet.
    auto runningStats = scalarBufferPool.getRunningStateStatistics();
    if ( NUM_BLOCKS != runningStats.size )
    {
        std::cout << "Block Pool runningStats.size is " << runningStats.size
            << " and should be " << NUM_BLOCKS << std::endl;
        return 2;
    }
    if ( NUM_BLOCKS != runningStats.lowWatermark )
    {
        std::cout << "Block Pool runningStats.lowWatermark is " << runningStats.lowWatermark
                  << " and should be " << NUM_BLOCKS << std::endl;
        return 3;
    }
    if ( NUM_BLOCKS != runningStats.runningCount )
    {
        std::cout << "Block Pool runningStats.runningCount is " << runningStats.runningCount
                  << " and should be " << NUM_BLOCKS << std::endl;
        return 4;
    }

    // We are going to scope the 'getting' a couple of blocks, so we can verify
    // that the smart pointer returned, delivers memory back to the pool when it
    // goes out of scope.
    {
        // Get a block and verify the running stats change.
        // Both the running count (blocks available) and the low watermark should
        // go down by one.
        auto pBlock1 = scalarBufferPool.getBlock();
        runningStats = scalarBufferPool.getRunningStateStatistics();
        if ( NUM_BLOCKS-1 != runningStats.lowWatermark )
        {
            std::cout << "Block Pool runningStats.lowWatermark is " << runningStats.lowWatermark
                      << " and should be " << NUM_BLOCKS-1 << std::endl;
            return 5;
        }
        if ( NUM_BLOCKS-1 != runningStats.runningCount )
        {
            std::cout << "Block Pool runningStats.runningCount is " << runningStats.runningCount
                      << " and should be " << NUM_BLOCKS-1 << std::endl;
            return 6;
        }

        // Get a block and verify the running stats change.
        // Both the running count (blocks available) and the low watermark should
        // go down by one.
        auto pBlock2 = scalarBufferPool.getBlock();
        runningStats = scalarBufferPool.getRunningStateStatistics();
        if ( NUM_BLOCKS-2 != runningStats.lowWatermark )
        {
            std::cout << "Block Pool runningStats.lowWatermark is " << runningStats.lowWatermark
                      << " and should be " << NUM_BLOCKS-2 << std::endl;
            return 7;
        }
        if ( NUM_BLOCKS-2 != runningStats.runningCount )
        {
            std::cout << "Block Pool runningStats.runningCount is " << runningStats.runningCount
                      << " and should be " << NUM_BLOCKS-2 << std::endl;
            return 8;
        }

        // We will test that the block size (number of elements) is big enough to hold
        // what we asked for. We will do this by looking at the raw pointer difference
        // in bytes between the two blocks we fetched.
        auto * pRaw1 = reinterpret_cast< u_char * >( pBlock1.get() );
        auto * pRaw2 = reinterpret_cast< u_char * >( pBlock2.get() );
        const size_t nBlockBytes = pRaw2 - pRaw1;
        if ( nBlockBytes < sizeof( double ) * NUM_ELEMENTS )
        {
            std::cout << "Block Byte Length is " << nBlockBytes
                    << " and should be " << sizeof( double ) * NUM_ELEMENTS << std::endl;
            return 9;
        }

        // Allow both smart pointers returned by `getBlock` to go out of scope
    }

    // The low watermark should remain unchanged, and the running count should return to maximum.
    runningStats = scalarBufferPool.getRunningStateStatistics();
    if ( NUM_BLOCKS-2 != runningStats.lowWatermark )
    {
        std::cout << "Block Pool runningStats.lowWatermark is " << runningStats.lowWatermark
                  << " and should be " << NUM_BLOCKS-2 << std::endl;
        return 10;
    }
    if ( NUM_BLOCKS != runningStats.runningCount )
    {
        std::cout << "Block Pool runningStats.runningCount is " << runningStats.runningCount
                  << " and should be " << NUM_BLOCKS << std::endl;
        return 11;
    }

    // Verify we can create a shared pointer type from the type return by `getBlock`
    {
        using SharedPtrType = std::shared_ptr< double[] >;
        SharedPtrType sharedPtrType{ scalarBufferPool.getBlock() };
        runningStats = scalarBufferPool.getRunningStateStatistics();
        if ( NUM_BLOCKS-1 != runningStats.runningCount )
        {
            std::cout << "Block Pool runningStats.runningCount is " << runningStats.runningCount
                      << " and should be " << NUM_BLOCKS-1 << std::endl;
            return 12;
        }

        // Allow the shared pointer to go out of scope
    }

    // The running count should return to maximum.
    runningStats = scalarBufferPool.getRunningStateStatistics();
    if ( NUM_BLOCKS != runningStats.runningCount )
    {
        std::cout << "Block Pool runningStats.runningCount is " << runningStats.runningCount
                  << " and should be " << NUM_BLOCKS << std::endl;
        return 13;
    }

    // Verify that we can determine the number of elements within a block
    // from the smart pointer returned by get block.
    // We support this only though querying the smart pointer's deleter.
    auto pBlock = scalarBufferPool.getBlock();
    auto numElements = pBlock.get_deleter().getNumElements();
    if ( NUM_ELEMENTS != numElements )
    {
        std::cout << "Block Pool numElements is " << numElements
                  << " and should be " << NUM_ELEMENTS << std::endl;
        return 14;
    }

    return 0;
}

class NoThrowAggregateType
{
public:
    NoThrowAggregateType() noexcept {  ++objCount; };
    ~NoThrowAggregateType() noexcept { --objCount; };

    int a{1};
    int b{2};
    static int objCount;
};
int NoThrowAggregateType::objCount = 0;

int testNoThrowAggregateType()
{
    constexpr size_t nElements = 4;
    BlockPool< NoThrowAggregateType > aggregateBlockPool{ 2, nElements };

    // Get a block and verify constructor invoked N times.
    auto block = aggregateBlockPool.getBlock();
    if ( nElements != NoThrowAggregateType::objCount )
    {
        std::cout << "Aggregate Test, Expected NoThrowAggregateType::objCount of " << nElements
                  << ", detected " << NoThrowAggregateType::objCount << std::endl;
        return 21;
    }

    // Get a pointer to the first NoThrowAggregateType instance and iterate it
    // and verify attributes were constructed on block memory.
    auto p = block.get();
    for ( size_t i = 0; nElements != i; ++i )
    {
        if ( 1 != p->a )
        {
            std::cout << "Aggregate Test, iter #" << i << ", expected p->a of 1, detected " << p->a << std::endl;
            return 22;
        }
        if ( 2 != p->b )
        {
            std::cout << "Aggregate Test, iter #" << i << ", expected p->b of 2, detected " << p->b << std::endl;
            return 23;
        }
    }

    // Reset the smart pointer and verify NoThrowAggregateType instances are destroyed.
    block.reset();
    if ( 0 != NoThrowAggregateType::objCount )
        std::cout << "Aggregate Test, Expected NoThrowAggregateType::objCount of " << 0
                  << ", detected " << NoThrowAggregateType::objCount << std::endl;

    return 0;
}

class ThrowOnThirdInstance
{
public:
    ThrowOnThirdInstance() { if ( 2 == objCount ) throw std::runtime_error( "BAH" ); ++objCount; ++everObjCount; }
    ~ThrowOnThirdInstance() noexcept { --objCount; }

    int a{1};
    int b{2};
    static int objCount;
    static int everObjCount;
};
int ThrowOnThirdInstance::objCount = 0;
int ThrowOnThirdInstance::everObjCount = 0;

int testThrowableAggregateType()
{
    constexpr size_t nElements = 4;
    BlockPool< ThrowOnThirdInstance > aggregateBlockPool{2, nElements };

    // We try this as we expect to catch an exception
    try
    {
        auto pBlock = aggregateBlockPool.getBlock();

        // If here, this is a failure of the test itself.
        std::cout << "Expected to catch an exception that did not occur. Fix the test!" << std::endl;
        return 31;

    } catch ( const std::runtime_error & )
    {
    }

    // The block pool should have experience a fetching of a raw block and having
    // it immediately returned because of the exception. We should see this with
    // the low watermark
    auto runningStats = aggregateBlockPool.getRunningStateStatistics();
    if ( 1 != runningStats.lowWatermark )
    {
        std::cout << "Block Pool runningStats.lowWatermark is " << runningStats.lowWatermark
                  << " and should be " << 1 << std::endl;
        return 32;
    }
    if ( 2 != runningStats.runningCount )
    {
        std::cout << "Block Pool runningStats.runningCount is " << runningStats.runningCount
                  << " and should be " << 2 << std::endl;
        return 33;
    }

    // The Object counters for the ThrowOnThirdInstance should indicate that 2 out of 4 were created
    // but also that those two were destroyed
    if ( 2 != ThrowOnThirdInstance::everObjCount )
    {
        std::cout << "Block Pool did create any objects before getBlock exception" << std::endl;
        return 34;
    }
    if ( 0 != ThrowOnThirdInstance::objCount )
    {
        std::cout << "Block Pool did not destroy all objects on getBlock exception" << std::endl;
        return 35;
    }

    return 0;
}

// The MemoryPoolBase has been thoroughly tested with ObjectPool. We will not repeat all of that here.
int main()
{
    int retVal;

    // Test with simple scalar types
    if ( 0 != ( retVal = testWithScalars() ) )
        return retVal;

    // Test with no-throw, aggregate type
    if ( 0 != ( retVal = testNoThrowAggregateType() ) )
        return retVal;

    // Test with throwing, aggregate type to prove invariance.
    if ( 0 != ( retVal = testThrowableAggregateType() ) )
        return retVal;

    return 0;
}