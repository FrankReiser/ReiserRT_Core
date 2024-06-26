/**
* @file MemoryPoolBase.cpp
* @brief The Specification for Generic Object Pool Base
*
* @authors: Frank Reiser
* @date Created on Feb 17, 2021
*/

#include "MemoryPoolBase.hpp"

#include "RingBufferSimple.hpp"

#include "Mutex.hpp"

#include <atomic>
#include <mutex>

using namespace ReiserRT::Core;

#include <cstring>     // For memset operation.

class ReiserRT_Core_EXPORT MemoryPoolBase::Imple
{
private:
    /**
    * @brief Friend Declaration
    *
    * We only allow our Outer class MemoryPoolBase to invoke our operations.
    */
    friend MemoryPoolBase;

    /**
    * @brief Internal RingBuffer Type
    *
    * This is the RingBuffer type we will utilize to store raw memory blocks from the memory arena.
    */
    using RingBufferType = RingBufferSimple< void * >;

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
        struct Counts
        {
            CounterType runningCount;   //!< The Current Running Count Captured Atomically (snapshot)
            CounterType lowWatermark;   //!< The Current Low Watermark Captured Atomically (snapshot)
        } counts;

        /**
        * @brief Overall State Variable
        *
        * This attribute will be assigned an atomically capture value of the same size.
        * It is also aliased with the anonymous structure that contains the piecemeal information
        * so that it may be read or written as a whole with an atomic guarantee.
        */
        RunningStateBasis state{ 0 };
    };

public:
    /**
    * @brief Default Constructor for MemoryPoolBase::Imple
    *
    * Default construction of MemoryPoolBase::Imple is disallowed. Hence, this operation has been deleted.
    */
    Imple() = delete;

private:
    /**
    * @brief Qualified Constructor for MemoryPoolBase::Imple
    *
    * This qualified constructor builds an MemoryPoolBase::Imple using the requestedNumElements and element size argument values.
    * It first constructs a empty RingBuffer to hold raw memory pointers using the argument value which
    * applies lower and upper limits, rounding upward to the next whole power of two.
    * It then allocates a block of memory, large enough to store all objects that may be created (the arena).
    * Then it populates the RingBuffer with pointers into the arena and initializes its running state statistics.
    *
    * @param requestedNumElements The requested ObjectPool size. This will be rounded up to the next whole
    * power of two and clamped within RingBuffer design limits.
    * @param theElementSize The size of each element.
    */
    explicit Imple( size_t requestedNumElements, size_t theElementSize )
      : ringBuffer{ requestedNumElements }
      , mutex{}
      , elementSize{ theElementSize }
      , paddedElementSize{ getPaddedTypeAllocSize( elementSize ) }
      , poolSize{ ringBuffer.getSize() }
      , runningState{}
      , arena{ new unsigned char [ theElementSize * poolSize ] }
    {
        // Stuff elements taken from the arena memory into the ring buffer.
        for ( size_t i = 0; i != poolSize; ++i )
            ringBuffer.put( reinterpret_cast< void* >( arena + i * theElementSize ) );

        // Initialize running state.
        InternalRunningStateStats runningStats;
        runningStats.counts.lowWatermark = runningStats.counts.runningCount = CounterType( poolSize );
        runningState = runningStats.state;
    }

public:
    /**
    * @brief Copy Constructor for MemoryPoolBase::Imple
    *
    * Copying MemoryPoolBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another Another instance of a MemoryPoolBase::Imple.
    */
    Imple( const Imple & another ) = delete;

    /**
    * @brief Copy Assignment Operation for MemoryPoolBase::Imple
    *
    * Copying MemoryPoolBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another Another instance of a MemoryPoolBase::Imple of the same template type.
    */
    Imple & operator =( const Imple & another ) = delete;

    /**
    * @brief Move Constructor for MemoryPoolBase::Imple
    *
    * Moving MemoryPoolBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another An rvalue reference to another instance of a MemoryPoolBase::Imple of the same template type.
    */
    Imple( Imple && another ) = delete;

    /**
    * @brief Move Assignment Operation for MemoryPoolBase::Imple
    *
    * Moving MemoryPoolBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another An rvalue reference to another instance of a MemoryPoolBase::Imple of the same template type.
    */
    Imple & operator =( Imple && another ) = delete;

private:
    /**
    * @brief Destructor for MemoryPoolBase::Imple
    *
    * The destructor destroys the memory arena.
    */
    ~Imple()
    {
        delete[] arena;
    }

    /**
    * @brief The Get Raw Block Operation
    *
    * This operation requests a block of memory from the pool.
    *
    * @throw Throws std::underflow error if the memory pool has been exhausted.
    * @returns A pointer to the raw memory block.
    */
    void * getRawBlock()
    {
        // Utilize small block scoping for mutex lock to minimize lock time.
        void *pRaw;
        {
            // Take lock RAII style
            std::lock_guard<Mutex> lock(mutex);

            // Get raw memory
            pRaw = ringBuffer.get();
        }

        // Manage Watermark Logic
        InternalRunningStateStats runningStats;
        InternalRunningStateStats runningStatsNew;
        runningStats.state = runningState.load(std::memory_order_seq_cst);
        do {
            // Clone atomically captured state,
            // We will be decrementing the running count and may lower the low watermark.
            runningStatsNew.state = runningStats.state;
            if (--runningStatsNew.counts.runningCount < runningStats.counts.lowWatermark)
                runningStatsNew.counts.lowWatermark = runningStatsNew.counts.runningCount;

        } while (!runningState.compare_exchange_weak(runningStats.state, runningStatsNew.state,
                                                     std::memory_order_seq_cst, std::memory_order_seq_cst));

        // Zero out Arena Memory!
        memset(pRaw, 0, paddedElementSize);

        // Return raw memory
        return pRaw;
    }

    /**
    * @brief The Return Raw Block Operation
    *
    * This operation returns a block of memory to the pool for subsequent reuse.
    *
    * @param pRaw A pointer to the raw block of memory to return to the pool.
    */
    void returnRawBlock( void * pRaw ) noexcept
    {
        // Utilize small block scoping for mutex lock to minimize lock time.
        {
            // Take lock RAII style
            std::lock_guard< Mutex > lock( mutex );

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
            ++runningStatsNew.counts.runningCount;

        } while ( !runningState.compare_exchange_weak( runningStats.state, runningStatsNew.state,
                                                       std::memory_order_seq_cst, std::memory_order_seq_cst ) );
    }

    /**
    * @brief Get the Running State Statistics
    *
    * This operation provides for performance monitoring of the MemoryPoolBase::Imple. The data returned
    * is an atomically captured snapshot of the RunningStateStats. The "low watermark",
    * compared to the size can provide an indication of the exhaustion level.
    *
    * @return Returns a snapshot of internal RunningStateStats.
    */
    [[nodiscard]] RunningStateStats getRunningStateStatistics() const noexcept
    {
        InternalRunningStateStats stats;
        stats.state = runningState;

        RunningStateStats snapshot;
        snapshot.size = poolSize;
        snapshot.runningCount = stats.counts.runningCount;
        snapshot.lowWatermark = stats.counts.lowWatermark;

        return snapshot;
    }

    /**
    * @brief Get the Padded Element Type Allocation Size
    *
    * This operation provides the element type allocation size which is padded to keep elements
    * properly aligned to the size of the architecture. It is static as it is used at time of
    * construction.
    *
    * @return Returns the padded element type allocation size which may be slightly larger than
    * the requested element size.
    */
    static size_t getPaddedTypeAllocSize( size_t requestedElementSize )
    {
        size_t alignmentOverspill = requestedElementSize % sizeof( void * );
        return  ( alignmentOverspill != 0 ) ? requestedElementSize + sizeof( void * ) - alignmentOverspill :
                requestedElementSize;
    }

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
    Mutex mutex;

    /**
    * @brief The Element Size
    *
    * This attribute stores the size of the elements managed by the pool, specified at time of construction.
    */
    const size_t elementSize;

    /**
    * @brief The Element Size
    *
    * This attribute stores the padded size of the elements managed by the pool, so that each element is properly
    * aligned with the architecture.
    */
    const size_t paddedElementSize;

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
    * it may be properly returned to the standard heap during MemoryPoolBase::Imple destruction.
    */
    alignas( void * ) unsigned char * arena;
};

MemoryPoolBase::MemoryPoolBase( size_t requestedNumElements, size_t elementSize )
  : pImple{ new Imple{ requestedNumElements, elementSize } }
{
}

MemoryPoolBase::~MemoryPoolBase()
{
    delete pImple;
}

void * MemoryPoolBase::getRawBlock()
{
    return pImple->getRawBlock();
}

void MemoryPoolBase::returnRawBlock( void * pRaw ) noexcept
{
    pImple->returnRawBlock( pRaw );
}

size_t MemoryPoolBase::getSize() const noexcept
{
    return pImple->poolSize;
}

size_t MemoryPoolBase::getElementSize() const noexcept
{
    return pImple->elementSize;
}

size_t MemoryPoolBase::getPaddedElementSize() const noexcept
{
    return pImple->paddedElementSize;
}

MemoryPoolBase::RunningStateStats MemoryPoolBase::getRunningStateStatistics() const noexcept
{
    return pImple->getRunningStateStatistics();
}
