/**
* @file ObjectPool.cpp
* @brief The Specification for ObjectPool
*
* @authors: Frank Reiser
* @date Created on Feb 17, 2021
*/

#include "ReiserRT_Core/ObjectPool.hpp"

/**
* @brief Macro OBJECT_POOL_USES_MUTEX_AND_SIMPLE_RING_BUFFER
*
* The lock free RingBuffer has a vulnerability with multiple getter threads with the near "empty" condition
* due to "postponed completions" resulting underflow. This happens when one a getter thread gets preempted
* and not updating completing the postponing record. A similar issue can happen on near full conditions
* with putter threads resulting in overflow. Basically, in order to use the lock free RingBuffer successfully,
* there should be no more than one getter thread and one putter thread. This can not be guaranteed that
* some future maintainer of code is unaware.
*/
#define OBJECT_POOL_USES_MUTEX_AND_SIMPLE_RING_BUFFER 1

#include "ReiserRT_Core/RingBufferSimple.hpp"

#include <atomic>
#include <mutex>

using namespace ReiserRT::Core;

/**
* @brief Macro OBJECT_POOL_INITIALIZES_RAW_MEMORY
*
* If this macro value is defined or defined as non-zero, then the ObjectPoolBase implementation
* will initialize all raw memory to zero. This would occur getting raw memory from the pool.
*/
#define OBJECT_POOL_INITIALIZES_RAW_MEMORY 1

#if defined( OBJECT_POOL_INITIALIZES_RAW_MEMORY )
#include <string.h>     // For memset operation.
#endif

class ObjectPoolBase::Imple
{
private:
    /**
    * @brief Friend Declaration
    *
    * We only allow our Outer class ObjectPoolBase to invoke our operations.
    */
    friend ObjectPoolBase;

    /**
    * @brief Internal RingBuffer Type
    *
    * This is the RingBuffer type we will utilize to store raw memory blocks from the memory arena.
    */
    using RingBufferType = RingBufferSimple< void * >;

    /**
    * @brief Mutex Type
    *
    * This is the Mutex type we will utilize protect the ring buffer from simultaneous access by multiple threads.
    */
    using MutexType = std::mutex;

    /**
    * @brief Running State Basis
    *
    * This needs to be twice the size of the CounterType as we will store two such types within one of these.
    */
    using RunningStateBasis = uint64_t;

    /**
    * @brief Atomic Running State Type
    *
    * This is a C++11 atomic variable. It supports reads and writes in an thread safe manner with minimal supervision
    * required for piecemeal updating. Being atomic affords lock-free, coherent access, to all bits of information contained within.
    */
    using RunningStateType = std::atomic< RunningStateBasis >;

    /**
    * @brief An Internal Type Running State Statistics.
    *
    * This type pairs, within the same memory region, two values, aliased to one value equal to their combined size.
    * It is used to atomically work with the larger, to set or scrutinize the smaller within it.
    * It's primary purpose is to provide ObjectPool performance characteristics in a lock-free manner.
    */
    union InternalRunningStateStats
    {
        /**
        * @brief Anonymous Structure Containing Two Counters
        *
        * This anonymous structure encapsulates a "Running Count" and a "Low Watermark".
        * The "Low Watermark", compared with the ObjectPool size can provide an indication
        * of the maximum exhaustion level ever achieved on an ObjectPool.
        */
        struct
        {
            CounterType runningCount;   //!< The Current Running Count Captured Atomically (snapshot)
            CounterType lowWatermark;   //!< The Current Low Watermark Captured Atomically (snapshot)
        };

        /**
        * @brief Overall State Variable
        *
        * This attribute will be assigned an atomically capture value of the same size.
        * It is also aliased with the anonymous structure that contains the piecemeal information
        * so that it may be read or written as a whole with an atomic guarantee.
        */
        RunningStateBasis state{ 0 };
    };

    /**
    * @brief Default Constructor for ObjectPoolBase::Imple
    *
    * Default construction of ObjectPoolBase::Imple is disallowed. Hence, this operation has been deleted.
    */
    Imple() = delete;

    /**
    * @brief Qualified Constructor for ObjectPoolBase::Imple
    *
    * This qualified constructor builds an ObjectPoolBase::Imple using the requestedNumElements and element size argument values.
    * It first constructs a empty RingBuffer to hold raw memory pointers using the argument value which
    * applies lower and upper limits, rounding upward to the next whole power of two.
    * It then allocates a block of memory, large enough to store all objects that may be created (the arena).
    * Then it populates the RingBuffer with pointers into the arena and initializes its running state statistics.
    *
    * @param requestedNumElements The requested ObjectPool size. This will be rounded up to the next whole
    * power of two and clamped within RingBuffer design limits.
    * @param theElementSize The maximum size of each allocation block.
    */
    explicit Imple( size_t requestedNumElements, size_t theElementSize );

    /**
    * @brief Copy Constructor for ObjectPoolBase::Imple
    *
    * Copying ObjectPoolBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another Another instance of a ObjectPoolBase::Imple.
    */
    Imple( const Imple & another ) = delete;

    /**
    * @brief Copy Assignment Operation for ObjectPoolBase::Imple
    *
    * Copying ObjectPoolBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another Another instance of a ObjectPoolBase::Imple of the same template type.
    */
    Imple & operator =( const Imple & another ) = delete;

    /**
    * @brief Move Constructor for ObjectPoolBase::Imple
    *
    * Moving ObjectPoolBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another An rvalue reference to another instance of a ObjectPoolBase::Imple of the same template type.
    */
    Imple( Imple && another ) = delete;

    /**
    * @brief Move Assignment Operation for ObjectPoolBase::Imple
    *
    * Moving ObjectPoolBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another An rvalue reference to another instance of a ObjectPoolBase::Imple of the same template type.
    */
    Imple & operator =( Imple && another ) = delete;

    /**
    * @brief Destructor for ObjectPoolBase::Imple
    *
    * The destructor destroys the memory arena.
    */
    ~Imple();

    /**
    * @brief The Get Raw Block Operation
    *
    * This operation requests a block of memory from the pool.
    *
    * @throw Throws std::underflow error if the memory pool has been exhausted.
    * @returns A pointer to the raw memory block.
    */
    void * getRawBlock();

    /**
    * @brief The Return Raw Block Operation
    *
    * This operation returns a block of memory to the pool for subsequent reuse.
    *
    * @param pRaw A pointer to the raw block of memory to return to the pool.
    */
    void returnRawBlock( void * pRaw ) noexcept;

    /**
    * @brief Get the ObjectPool size
    *
    * This operation retrieves the fixed size of the ObjectPoolBase::Imple determined at the time of construction.
    *
    * @return Returns the ObjectPoolBase::Imple fixed size determined at the time of construction.
    */
    size_t getSize() noexcept;

    /**
    * @brief Get the Running State Statistics
    *
    * This operation provides for performance monitoring of the ObjectPoolBase::Imple. The data returned
    * is an atomically captured snapshot of the RunningStateStats. The "low watermark",
    * compared to the size can provide an indication of the exhaustion level.
    *
    * @return Returns a snapshot of internal RunningStateStats.
    */
    RunningStateStats getRunningStateStatistics() noexcept;

    /**
    * @brief Our RingBuffer
    *
    * This attribute is our RingBuffer. RingBuffers are of a circular nature.
    * This one is pre-populated with raw memory pointers into the memory arena.
    */
    RingBufferType ringBuffer;

    /**
    * @brief The Mutex
    *
    * This contains an instance of a POSIX mutex on Linux systems.
    * Mutexes are designed to provide mutual exclusion around sections of code to ensure coherency
    * of data accessed in multi-threaded environments.
    */
    MutexType mutex;

#if defined( OBJECT_POOL_INITIALIZES_RAW_MEMORY) && ( OBJECT_POOL_INITIALIZES_RAW_MEMORY != 0 )
    /**
    * @brief The Element Size
    *
    * This attribute stores the size of the elements managed by the pool.
    */
    const size_t elementSize;
#endif

    /**
    * @brief The Pool Size
    *
    * This attribute records the RingBuffer size determined at construction.
    */
    const size_t poolSize;

    /**
    * @brief The RunningState Type
    *
    * This attribute holds 64 bits of atomically managed running state information. It is updated
    * on each getRawBlock and returnRawBlock invocation.
    */
    RunningStateType runningState;

    /**
    * @brief Our Memory Arena
    *
    * This attribute records the pointer to our memory arena that was allocated during construction so that
    * it may be properly returned to the standard heap during ObjectPoolBase::Imple destruction.
    */
    alignas( void * ) unsigned char * arena;
};

ObjectPoolBase::Imple::Imple( size_t requestedNumElements, size_t theElementSize )
        : ringBuffer{ requestedNumElements }
        , mutex{}
#if defined( OBJECT_POOL_INITIALIZES_RAW_MEMORY) && ( OBJECT_POOL_INITIALIZES_RAW_MEMORY != 0 )
        , elementSize{ theElementSize }
#endif
        , poolSize{ ringBuffer.getSize() }
        , runningState{}
        , arena{ new unsigned char [ theElementSize * poolSize ] }
{
    // Stuff elements taken from the arena memory into the ring buffer.
    for ( size_t i = 0; i != poolSize; ++i )
        ringBuffer.put( reinterpret_cast< void* >( arena + i * theElementSize ) );

    // Initialize running state.
    InternalRunningStateStats runningStats;
    runningStats.lowWatermark = runningStats.runningCount = CounterType( poolSize );
    runningState = runningStats.state;
}

ObjectPoolBase::Imple::~Imple()
{
    delete[] arena;
}

void * ObjectPoolBase::Imple::getRawBlock()
{
    // Utilize small block scoping for mutex lock to minimize lock time.
    void * pRaw = nullptr;
    {
        // Take lock RAII style
        std::lock_guard< MutexType > lock( mutex );

        // Get raw memory
        pRaw = ringBuffer.get();
    }

    // Manage Watermark Logic
    InternalRunningStateStats runningStats;
    InternalRunningStateStats runningStatsNew;
    runningStats.state = runningState.load( std::memory_order_seq_cst );
    do
    {
        // Clone atomically captured state,
        // We will be decrementing the running count and may lower the low watermark.
        runningStatsNew.state = runningStats.state;
        if ( --runningStatsNew.runningCount < runningStats.lowWatermark )
            runningStatsNew.lowWatermark = runningStatsNew.runningCount;

    } while ( !runningState.compare_exchange_weak( runningStats.state, runningStatsNew.state,
                                                   std::memory_order_seq_cst, std::memory_order_seq_cst ) );

#if defined( OBJECT_POOL_INITIALIZES_RAW_MEMORY) && ( OBJECT_POOL_INITIALIZES_RAW_MEMORY != 0 )
    // Zero out Arena Memory!
    memset( pRaw, 0, elementSize );
#endif

    // Return raw memory
    return pRaw;
}

void ObjectPoolBase::Imple::returnRawBlock( void * pRaw ) noexcept
{
    // Utilize small block scoping for mutex lock to minimize lock time.
    {
        // Take lock RAII style
        std::lock_guard< MutexType > lock( mutex );

        // Return raw memory back to the pool.
        ringBuffer.put( pRaw );
    }

    // Manage Watermark Logic
    InternalRunningStateStats runningStats;
    InternalRunningStateStats runningStatsNew;
    runningStats.state = runningState.load( std::memory_order_seq_cst );
    do
    {
        // Clone atomically captured state,
        // We will be incrementing the running count and not touching the low watermark.
        runningStatsNew.state = runningStats.state;
        ++runningStatsNew.runningCount;

    } while ( !runningState.compare_exchange_weak( runningStats.state, runningStatsNew.state,
                                                   std::memory_order_seq_cst, std::memory_order_seq_cst ) );
}

size_t ObjectPoolBase::Imple::getSize() noexcept
{
    return poolSize;
}

ObjectPoolBase::RunningStateStats ObjectPoolBase::Imple::getRunningStateStatistics() noexcept
{
    InternalRunningStateStats stats;
    stats.state = runningState;

    RunningStateStats snapshot;
    snapshot.size = poolSize;
    snapshot.runningCount = stats.runningCount;
    snapshot.lowWatermark = stats.lowWatermark;

    return snapshot;
}

ObjectPoolBase::ObjectPoolBase( size_t requestedNumElements, size_t elementSize )
        : pImple{ new Imple{ requestedNumElements, elementSize } }
{
}

ObjectPoolBase::~ObjectPoolBase()
{
    delete pImple;
}

void * ObjectPoolBase::getRawBlock()
{
    return pImple->getRawBlock();
}

void ObjectPoolBase::returnRawBlock( void * pRaw ) noexcept
{
    pImple->returnRawBlock( pRaw );
}

size_t ObjectPoolBase::getSize() noexcept
{
    return pImple->getSize();
}

ObjectPoolBase::RunningStateStats ObjectPoolBase::getRunningStateStatistics() noexcept
{
    return pImple->getRunningStateStatistics();
}
