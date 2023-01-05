//
// Created by frank on 1/5/23.
//

#include "BlockPool.hpp"

#include <iostream>

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

    // We are going to scope the 'getting' a couple of block, so we can verify
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

        // Allow both smart pointer returned by `getBlock` to go out of scope
    }

    // The low watermark should remain unchanged and the running count should return to maximum.
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

    return 0;
}

// The MemoryPoolBase has been tested thoroughly with ObjectPool. We will not repeat all of that here.
int main()
{
    int retVal = 0;

    do {
        // Test with simple scalar types
        if ( 1 != ( retVal = testWithScalars() ) )
            break;

        ///@todo Test With Aggregate Type that does not throw

        ///@todo Test With Aggregate Type that does throw

    } while (false);

    return retVal;
}