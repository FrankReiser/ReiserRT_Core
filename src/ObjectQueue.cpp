/**
* @file ObjectQueue.cpp
* @brief The Implementation file for a Pendable ObjectQueue
* @authors Frank Reiser
* @date Created on Apr 10, 2017
*/

#include "ReiserRT_Core/ObjectQueue.hpp"
#include "ReiserRT_Core/RingBufferGuarded.hpp"

#define OBJECT_QUEUE_INITIALIZES_RAW_MEMORY 1

#if defined( OBJECT_QUEUE_INITIALIZES_RAW_MEMORY) && ( OBJECT_QUEUE_INITIALIZES_RAW_MEMORY != 0 )
#include <string.h>     // For memset operation.
#endif


using namespace ReiserRT::Core;

/**
* @brief ObjectQueueBase Implementation Class
*
* This class provides for ObjectQueueBase class' hidden implementation.
*/
class ObjectQueueBase::Imple
{
private:
    /**
    * @brief Friend Declaration
    *
    * We only allow our Outer class ObjectQueueBase to invoke our operations.
    */
    friend ObjectQueueBase;

    /**
    * @brief Ring Buffer Type
    *
    * We utilize a guarded buffer type of generic void pointers for both our raw and cooked
    * ring buffers. We utilize this Guarded/Counted ring buffer type as it encapsulates a counted semaphore
    * with both manages thread safety and efficient sleeping until data is available for retrieving.
    */
    using RingBufferType = ReiserRT::Core::RingBufferGuarded< void * >;

    /**
    * @brief Running State Basis
    *
    * This needs to be twice the size of the CounterType as we will store two such types within one of these.
    */
    using RunningStateBasis = uint64_t;

    /**
    * @brief Atomic Running State Type
    *
    * This is a C++11 atomic variable type. It supports reads and writes in an thread safe manner with minimal supervision
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
        * This anonymous structure encapsulates a "Running Count" and a "High Watermark".
        * The "High Watermark", compared with the ObjectQueue size can provide an indication
        * of the maximum exhaustion level ever achieved on an ObjectPool.
        */
        struct
        {
            CounterType runningCount;    //!< The Current Running Count Captured Atomically (snapshot)
            CounterType highWatermark;   //!< The Current High Watermark Captured Atomically (snapshot)
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
    * @brief Default Constructor for ObjectQueueBase::Imple
    *
    * Default construction of ObjectQueueBase::Imple is disallowed. Hence, this operation has been deleted.
    */
    Imple() = delete;

    /**
    * @brief Qualified ObjectQueueBase::Imple Constructor
    *
    * This constructor build the ObjectQueueBase::Imple of exactly the requested size. Its internal
    * RingBuffer objects will be up-sized to the next power of two. It instantiates two RingBuffer
    * objects, two semaphores and allocates an arena large enough for a worst case load of put/emplace operations
    * in lieu of a single get operation.
    *
    * @param requestedNumElements The number of elements requested for the ObjectQueue.
    * @param theElementSize The size of each element.
    *
    * @throw Throws std::bad_alloc if memory requirements for the arena, or ring buffer internals cannot be statisfied.
    */
    Imple( size_t requestedNumElements, size_t theElementSize );

    /**
    * @brief Copy Constructor for ObjectQueueBase::Imple
    *
    * Copying ObjectQueueBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another Another instance of a ObjectQueueBase::Imple.
    */
    Imple( const Imple & another ) = delete;

    /**
    * @brief Copy Assignment Operation for ObjectQueueBase::Imple
    *
    * Copying ObjectQueueBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another Another instance of a ObjectQueueBase::Imple.
    */
    Imple & operator =( const Imple & another ) = delete;

    /**
    * @brief Move Constructor for ObjectQueueBase::Imple
    *
    * Moving ObjectQueueBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another An rvalue reference to another instance of a ObjectQueueBase::Imple.
    */
    Imple( Imple && another ) = delete;

    /**
    * @brief Move Assignment Operation for ObjectQueueBase::Imple
    *
    * Moving ObjectQueueBase::Imple is disallowed. Hence, this operation has been deleted.
    *
    * @param another An rvalue reference to another instance of a ObjectQueueBase::Imple.
    */
    Imple & operator =( Imple && another ) = delete;

    /**
    * @brief Destructor for ObjectQueueBase::Imple
    *
    * This destructor returns the raw memory arena to the standard heap.
    */
    ~Imple();

    /**
    * @brief The abort Operation
    *
    * This operation is invoked to abort the ObjectQueueBase::Imple. It first aborts the cookedSemaphore and then
    * it aborts the rawSemaphore.
    *
    * @note Once aborted, there is no recovery. Further put, emplace or get operations will receive a Semaphore::AbortException.
    */
    void abort()
    {
        aborted = true;
        cookedRingBuffer.abort();
        rawRingBuffer.abort();
    }

    /**
    * @brief The Raw Get Operation
    *
    * This operation is invoked to obtain raw memory from the "raw" ring buffer. It may block
    * waiting on the raw ring buffer. When data is available, it is retrieved and the running state
    * is updated.
    *
    * @throw Throws std::logic_error if the "raw" ring buffer is not in the "Ready" state.
    * @throw Throws std::runtime_error if the "raw" ring buffer becomes aborted.
    * @throw Throws std::underflow exception if there is no element available to forefill the request (empty).
    * Being that this is a "guarded" ring buffer. This would be indicative an implementation error within the
    * guarded ring buffer implementation.
    *
    * @return Returns a void pointer to raw memory from the raw ring buffer.
    */
    void * rawWaitAndGet();

    /**
    * @brief The Cooked Put Operation
    *
    * This operation loads the "cooked" ring buffer with a pointer that references some sort of
    * object of unknown type.
    *
    * @param pCooked A pointer to an object of unknown type.
    *
    * @throw Throws std::logic_error if the "cooked" ring buffer is not in the "Ready" state.
    * @throw Throws std::overflow exception if there is no room left to forefill the request (full).
    * We manage this by not putting anything in the "cooked" ring buffer that we couldn't forefill from the
    * "raw" ring buffer. Therefore, this exception would be indicative of an implementation problem within this class.
    * @throw Throws std::runtime_error if the "cooked" ring buffer becomes aborted.
    */
    inline void cookedPutAndNotify( void * pCooked )
    {
        // Put cooked memory into the cooked ring buffer. The guarded ring buffer performs the notification.
        cookedRingBuffer.put( pCooked );
    }

    /**
    * @brief The Cooked Get Operation
    *
    * This operation gets "cooked" memory from the "cooked" ring buffer.  It may block
    * waiting on the "cooked" ring buffer. When data is available, it is retrieved.
    *
    * @throw Throws std::runtime_error if the "cooked" ring buffer becomes aborted.
    * @throw Throws std::logic_error if the "cooked" ring buffer is not in the "Ready" state.
    * @throw Throws std::underflow exception if there is no element available to fulfill the request (empty).
    * Being that this is a "guarded" ring buffer. This would be indicative an implementation error within the
    * guarded ring buffer implementation.
    *
    * @return Returns memory for an object of unknown type from the cooked ring buffer.
    */
    inline void * cookedWaitAndGet()
    {
        // Get cooked memory from the cooked ring buffer. The guarded ring buffer performs the wait.
        return cookedRingBuffer.get();
    }

    /**
    * @brief The Raw Put Operation
    *
    * This operation loads the raw ring buffer with memory that is available to use for cooking.
    * It then notifies the raw semaphore of resource availability.
    *
    * @param pRaw A pointer to memory that is available for cooking.
    *
    * @throw Throws std::logic_error if the "raw" ring buffer is not in the "Ready" state.
    * @throw Throws std::runtime_error if the "raw" ring buffer becomes aborted.
    * @throw Throws std::overflow exception if there is no room left to fulfill the request (full).
    * We manage this by not putting anything in the "raw" ring buffer that we couldn't fulfill from the
    * "cooked" ring buffer. Therefore, this exception would be indicative of an implementation problem within this class.
    */
    void rawPutAndNotify( void * pRaw );

    /**
    * @brief The Flush Operation
    *
    * This operation is provided to empty the "cooked" ring buffer, and properly destroy any unfetched objects that may remain.
    * Being that we only deal with void pointers at this level, what do not know what type of destructor to invoke.
    * However, through a user provided function object, the destruction can be elevated to the client level, which should know
    * what type of base objects may remain.
    *
    * @pre The "cooked" ring buffer is expected to be in the "Terminal" state to invoke this operation. Violations will result in an exception
    * being thrown.
    *
    * @param operation This is a functor value of type ObjectQueueBase::FlushingFunctionType. It must have a valid target
    * (i.e., an empty function object). It will be invoked once per remaining object in the "cooked" ring buffer.
    *
    * @throw Throws std::logic_error if the "cooked" ring buffer is not in the "Terminal" state.
    * @throw Throws std::bad_function_call if the operation passed in has no target (an empty function object).
    */
    inline void flush( ObjectQueueBase::FlushingFunctionType & operation )
    {
        auto funk = [ operation ]( void * pV ) noexcept { operation( pV ); };
        cookedRingBuffer.flush( std::ref( funk ) );
    }

    /**
    * @brief The Get Running State Statistics Operation
    *
    * This operation provides for performance monitoring of the ObjectQueue. The data returned
    * is an atomically captured snapshot of the RunningStateStats. The "high watermark",
    * compared to the size can provide an indication of the queue maximum usage level.
    *
    * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked.
    *
    * @return Returns a snapshot of InternalRunningStateStats.
    */
    RunningStateStats getRunningStateStatistics() noexcept;

    /**
    * @brief The Actual Requested Size
    *
    * This attribute records the actual requested size. It may be less than that allocated
    * to RingBuffers as they always round up to the next power of two.
    */
    const size_t requestedSize;

    /**
    * @brief The Raw Ring Buffer
    *
    * This is our RingBuffer where raw memory pointers reside waiting to be retrieved and "cooked".
    */
    RingBufferType rawRingBuffer;

    /**
    * @brief The Cooked Ring Buffer
    *
    * This is our RingBuffer where "cooked" objects are stored until they are retrieved
    * by the get operation.
    */
    RingBufferType cookedRingBuffer;

    /**
    * @brief The RunningState Type
    *
    * This attribute holds 64 bits of atomically managed running state information. It is updated
    * on each get, put or emplace invocation and is available for performance monitoring clients
    * through the getRunningStateStatistics operation.
    */
    RunningStateType runningState;

#if defined( OBJECT_QUEUE_INITIALIZES_RAW_MEMORY) && ( OBJECT_QUEUE_INITIALIZES_RAW_MEMORY != 0 )
    /**
    * @brief The Element Size
    *
    * This attribute stores the size of the elements managed by the pool.
    */
    const size_t elementSize;
#endif

    /**
    * @brief Our Memory Arena
    *
    * This attribute records the pointer to our memory arena that was allocated during construction so that
    * it may be properly returned to the standard heap during ObjectQueue destruction.
    */
    alignas( void * ) unsigned char * arena;  // I.e., Types not constructed.


    /**
    * @brief The Aborted Indicator
    *
    * This attribute provides a quick means of detecting whether we have been aborted.
    */
    std::atomic_bool aborted;

};


ObjectQueueBase::Imple::Imple( size_t requestedNumElements, size_t theElementSize )
    : requestedSize{ requestedNumElements }
    , rawRingBuffer{ requestedNumElements, true }   // Ditto, plus we signify that we will prime the requested number of elements.
    , cookedRingBuffer{ requestedNumElements }      // Gets Rounded up to the next power of two.
    , runningState{}                                // Initialize our running state.
#if defined( OBJECT_QUEUE_INITIALIZES_RAW_MEMORY) && ( OBJECT_QUEUE_INITIALIZES_RAW_MEMORY != 0 )
    , elementSize{ theElementSize }
#endif
    , arena{ new unsigned char [ theElementSize * requestedNumElements ] }
    , aborted{ false }
{
    // Prime the raw ring buffer with void pointers into our arena space. We do this with a lambda function.
    auto funk = [ this, theElementSize ]( size_t i ) noexcept { return reinterpret_cast< void * >( arena + i * theElementSize ); };
    rawRingBuffer.prime( std::ref( funk ) );
}

ObjectQueueBase::Imple::~Imple()
{
    // Return our arena memory to the standard heap.
    delete[] arena;
}

void * ObjectQueueBase::Imple::rawWaitAndGet()
{
    // Get raw memory from the raw ring buffer. The guarded ring buffer performs the wait.
    void * pRaw = rawRingBuffer.get();

    // Manage running count and high water mark.
    InternalRunningStateStats runningStats;
    InternalRunningStateStats runningStatsNew;
    runningStats.state = runningState.load( std::memory_order_seq_cst );
    do
    {
        // Clone atomically captured state,
        // We will be incrementing the running count, may raise the high water mark.
        runningStatsNew.state = runningStats.state;
        if ( ++runningStatsNew.runningCount > runningStats.highWatermark )
            runningStatsNew.highWatermark = runningStatsNew.runningCount;

    } while ( !runningState.compare_exchange_weak( runningStats.state, runningStatsNew.state,
            std::memory_order_seq_cst, std::memory_order_seq_cst ) );

#if defined( OBJECT_QUEUE_INITIALIZES_RAW_MEMORY) && ( OBJECT_QUEUE_INITIALIZES_RAW_MEMORY != 0 )
    // Zero out Arena Memory!
    memset( pRaw, 0, elementSize );
#endif

    // Return Raw Block
    return pRaw;
}

void ObjectQueueBase::Imple::rawPutAndNotify( void * pRaw )
{
    // Put formerly cooked memory into the cooked ring buffer. The guarded ring buffer performs the notify.
    rawRingBuffer.put( pRaw );

    // Manage running count.
    InternalRunningStateStats runningStats;
    InternalRunningStateStats runningStatsNew;
    runningStats.state = runningState.load( std::memory_order_seq_cst );
    do
    {
        // Clone atomically captured state,
        // We will be decrementing the running count and not touching the high watermark.
        runningStatsNew.state = runningStats.state;
        --runningStatsNew.runningCount;

    } while ( !runningState.compare_exchange_weak( runningStats.state, runningStatsNew.state,
            std::memory_order_seq_cst, std::memory_order_seq_cst ) );
}

ObjectQueueBase::RunningStateStats ObjectQueueBase::Imple::getRunningStateStatistics() noexcept
{
    // Get the running state.
    InternalRunningStateStats stats;
    stats.state = runningState;

    // Initialize return snapshot value.
    RunningStateStats snapshot;
    snapshot.size = requestedSize;
    snapshot.runningCount = stats.runningCount;
    snapshot.highWatermark = stats.highWatermark;

    // Return the snapshot.
    return snapshot;
}

ObjectQueueBase::ObjectQueueBase( size_t requestedNumElements, size_t elementSize )
    : pImple{ new Imple{ requestedNumElements, elementSize } }
{
}

ObjectQueueBase::~ObjectQueueBase()
{
    delete pImple;
}

void ObjectQueueBase::abort()
{
    pImple->abort();
}

void * ObjectQueueBase::rawWaitAndGet()
{
    return pImple->rawWaitAndGet();
}

void ObjectQueueBase::cookedPutAndNotify( void * pCooked )
{
    pImple->cookedPutAndNotify( pCooked );
}

void * ObjectQueueBase::cookedWaitAndGet()
{
    return pImple->cookedWaitAndGet();
}

void ObjectQueueBase::rawPutAndNotify( void * pRaw )
{
    pImple->rawPutAndNotify( pRaw );
}

void ObjectQueueBase::flush( FlushingFunctionType operation )
{
    pImple->flush( operation );
}

ObjectQueueBase::RunningStateStats ObjectQueueBase::getRunningStateStatistics() noexcept
{
    return pImple->getRunningStateStatistics();
}

bool ObjectQueueBase::isAborted() noexcept
{
    return pImple->aborted;
}

