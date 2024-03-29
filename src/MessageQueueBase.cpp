/**
* @file MessageQueueBase.cpp
* @brief The Implementation file for MessageQueueBase
*
* This file came into existence in a effort to replace and ObjectPool usage and ObjectQueue implementation
* in an effort to eliminate an extra Mutex take/release on enqueueing and de-queueing of messages.
* ObjectQueue will be eliminated in this process.
*
* @authors Frank Reiser
* @date Created on Apr 6, 2021
*/

#include "MessageQueueBase.hpp"
#include "RingBufferGuarded.hpp"
#include "Mutex.hpp"

#include <cstring>
#include <thread>
#include <mutex>

using namespace ReiserRT::Core;

/**
* @brief MessageQueueBase Hidden Implementation
*
* This class maintains the details and provides the behavior required of MessageQueueBase that is
* not exposed at the MessageQueueBase API layer.
*/
class ReiserRT_Core_EXPORT MessageQueueBase::Imple
{
private:
    /**
    * @brief Friend Declaration
    *
    * Only instances of MessageQueueBase may access our private member data and operations.
    */
    friend class MessageQueueBase;

    /**
    * @brief The Raw Ring Buffer Type
    *
    * Here we define our raw ring buffer type
    */
    using RawRingBufferType = RingBufferGuarded< void * >;

    /**
    * @brief The Cooked Ring Buffer Type
    *
    * Here we define our "cooked" ring buffer type
    */
    using CookedRingBufferType = RingBufferGuarded< MessageBase * >;

    /**
    * @brief Mutex Pointer Type Declaration
    *
    * The mutex pointer type we will use.
    */
    using MutexPtrType = std::unique_ptr< Mutex >;

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
        struct Counts
        {
            CounterType runningCount;    //!< The Current Running Count Captured Atomically (snapshot)
            CounterType highWatermark;   //!< The Current High Watermark Captured Atomically (snapshot)
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

    /**
    * @brief Qualified MessageQueueBase::Imple Constructor
    *
    * This constructor builds the implementation. It instantiates two counted ring buffer objects.
    * One for raw memory and one for newly constructed "cooked" messages.
    *
    * @param requestedNumElements The number of elements requested for the ObjectQueue. This will be the exact
    * maximum amount of messages that can either be enqueued or dequeued before blocking occurs.
    * @param theElementSize The maximum size of an element.
    *
    * @throw Throws std::bad_alloc if memory requirements for the arena, or ring buffer internals cannot be satisfied.
    */
    Imple( std::size_t theRequestedNumElements, std::size_t theElementSize, bool enableDispatchLocking );

    /**
    * @brief Destructor for ObjectQueueBase::Imple
    *
    * This destructor returns the raw memory arena to the standard heap. This should not be invoked until our
    * client has shut us down with appropriate abort and flush. We do it like this as to ensure the appropriate
    * tear down of MessageQueue as it has many pieces and serves multiple threads.
    */
    ~Imple();

    /**
    * @brief The Get Running State Statistics Operation
    *
    * This operation provides for performance monitoring.
    *
    * @return Returns a snapshot of RunningStateStats.
    */
    RunningStateStats getRunningStateStatistics() noexcept;

    /**
    * @brief The abort Operation
    *
    * This operation is invoked to abort the implementation.
    *
    * @note Once aborted, there is no recovery. Get or put operations will receive a std::runtime_error.
    */
    void abort()
    {
        aborted = true;
        cookedRingBuffer.abort();
        rawRingBuffer.abort();
    }

    /**
    * @brief The rawWaitAndGet Operation
    *
    * This operation is invoked to obtain raw memory from the raw implementation.
    *
    * @throw Throws std::runtime_error if the "raw" ring buffer becomes aborted.
    *
    * @return Returns a void pointer to raw memory from the implementation.
    */
    void * rawWaitAndGet();

    /**
    * @brief The cookedPutAndNotify Operation
    *
    * This operation loads cooked memory into the implementation and notifies any waiters of availability.
    *
    * @param pCooked A pointer to an abstract MessageBase object.
    *
    * @throw Throws std::runtime_error if the "cooked" ring buffer becomes aborted.
    */
    inline void cookedPutAndNotify( MessageBase * pCooked )
    {
        // Put cooked memory into the cooked ring buffer. The guarded ring buffer performs the notification.
        cookedRingBuffer.put( pCooked );
    }

    /**
    * @brief The cookedWaitAndGet Operation
    *
    * This operation is invoked to obtain cooked memory from the implementation.
    *
    * @throw Throws std::runtime_error if the "cooked" ring buffer becomes aborted.
    *
    * @return Returns a pointer to an abstract MessageBase object from the implementation.
    */
    inline MessageBase * cookedWaitAndGet()
    {
        // Get cooked memory from the cooked ring buffer.
        return cookedRingBuffer.get();
    }

    /**
    * @brief The rawPutAndNotify Operation
    *
    * This operation restores raw memory back into the our implementation and notifies any
    * waiters for such availability.
    *
    * @param pRaw A pointer to raw memory that is to be made available for cooking.
    *
    * @throw Throws std::runtime_error if the "raw" ring buffer becomes aborted.
    */
    void rawPutAndNotify( void * pRaw );

    /**
    * @brief Dispatch a Message
    *
    * This wrapper operation stores the name of the message prior to dispatch. We do this in the case
    * that should the operation throw, the name that threw is recorded.
    * If dispatch locking has been enabled, we will lock the dispatch mutex just prior to dispatching the message.
    *
    * @param pMsg A pointer to the abstract MessageBase being dispatched.
    */
    void dispatchMessage( MessageBase * pMsg );

    /**
    * @brief Get the Name of the Last Message Dispatched.
    *
    * This is more of a diagnostic routine which may be helpful if one particular message type is bombing on dispatch.
    * In order to be useful, the MessageBase::name abstract should be overridden to provide something other than the default
    * implementation. Please see MessageBase::name default implementation for an example of how to implement an override.
    *
    * @return Returns the name of the last message dispatched.
    */
    const char * getNameOfLastMessageDispatched()
    {
        return nameOfLastMessageDispatched.load();
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
    static size_t getPaddedTypeAllocSize( size_t requestedElementSize );

    /**
    * @brief The Actual Requested Size
    *
    * This attribute records the actual requested size. It may be less than that allocated
    * to RingBuffers as they always round up to the next power of two.
    */
    const size_t requestedNumElements;

    /**
    * @brief The Element Size
    *
    * This attribute stores the maximum size of the elements managed by the pool.
    */
    const size_t elementSize;

    /**
    * @brief Mutex Instance
    *
    * Our mutex instance pointer and flag. If it is a nullptr, then dispatch locking is disabled and we
    * do not use it. Otherwise, we use it to lock while dispatching.
    */
    MutexPtrType pMutex;

    /**
    * @brief Our Memory Arena
    *
    * This attribute records the pointer to our memory arena that was allocated during construction so that
    * it may be properly returned to the standard heap during ObjectQueue destruction.
    */
    alignas( void * ) unsigned char * arena;

    /**
    * @brief The Name of the Last Message Dispatched
    *
    * This attribute maintains the name of the last message dispatched by the MessageQueue.
    * We use an atomic pointer so that it can be fetched in a thread safe manner.
    */
    std::atomic< const char * > nameOfLastMessageDispatched{ "[NONE]" };

    /**
    * @brief The Raw Ring Buffer
    *
    * This is our RingBuffer where raw memory pointers reside waiting to be retrieved and "cooked".
    */
    RawRingBufferType rawRingBuffer;

    /**
    * @brief The Cooked Ring Buffer
    *
    * This is our RingBuffer where "cooked" objects are stored until they are retrieved
    * by the get operation.
    */
    CookedRingBufferType cookedRingBuffer;

    /**
    * @brief The RunningState Type
    *
    * This attribute holds 64 bits of atomically managed running state information. It is updated
    * on each get, put or emplace invocation and is available for performance monitoring clients
    * through the getRunningStateStatistics operation.
    */
    RunningStateType runningState{0};

    /**
    * @brief The Aborted Indicator
    *
    * This attribute provides a quick means of detecting whether we have been aborted.
    */
    std::atomic_bool aborted{ false };
};

const char * MessageBase::name() const
{
    return "Unforgiven";
}

MessageQueueBase::AutoDispatchLock::AutoDispatchLock( MessageQueueBase * pTheMQB )
        : pMQB{ pTheMQB }
{
    // We shall not construct AutoDispatchLock with a null mutex pointer.
    // This is enforced by our client MessageQueueBase as it would throw before allowing it.
    pMQB->pImple->pMutex->lock();
}

MessageQueueBase::AutoDispatchLock::~AutoDispatchLock()
{
    // If we have not been "moved from", unlock. Otherwise, the "moved to" is responsible.
    if ( pMQB && locked ) pMQB->pImple->pMutex->unlock();
}

MessageQueueBase::AutoDispatchLock::NativeHandleType MessageQueueBase::AutoDispatchLock::native_handle()
{
    return pMQB ? pMQB->pImple->pMutex->native_handle() : nullptr;
}

void MessageQueueBase::AutoDispatchLock::lock()
{
    if ( pMQB ) {
        pMQB->pImple->pMutex->lock();
        locked = 1;
    }
}

void MessageQueueBase::AutoDispatchLock::unlock()
{
    if ( pMQB && locked ) {
        pMQB->pImple->pMutex->unlock();
        locked = 0;
    }
}

MessageQueueBase::Imple::Imple( std::size_t theRequestedNumElements, std::size_t theElementSize,
                                bool enableDispatchLocking )
  : requestedNumElements{ theRequestedNumElements }
  , elementSize{ getPaddedTypeAllocSize( theElementSize ) }
  , pMutex{ enableDispatchLocking ? new Mutex{} : nullptr }
  , arena{ new unsigned char [ elementSize * requestedNumElements ] }
  , rawRingBuffer{ theRequestedNumElements, true }
  , cookedRingBuffer{ theRequestedNumElements }
{
    // Prime the raw ring buffer with void pointers into our arena space. We do this with a lambda function.
    auto funk = [ this ]( size_t i )
            { return reinterpret_cast< void * >( arena + i * elementSize ); };
    rawRingBuffer.prime( funk );
}

MessageQueueBase::Imple::~Imple()
{
    // Flush anything left in the cooked ring buffer.
    auto funk = []( void * pV ) { auto * pM = reinterpret_cast< MessageBase * >( pV ); pM->~MessageBase(); };
    cookedRingBuffer.flush( funk );

    // Return our arena memory to the standard heap.
    delete[] arena;
}

MessageQueueBase::RunningStateStats MessageQueueBase::Imple::getRunningStateStatistics() noexcept
{
    // Get the running state.
    InternalRunningStateStats stats;
    stats.state = runningState;

    // Initialize return snapshot value.
    RunningStateStats snapshot;
    snapshot.size = requestedNumElements;
    snapshot.runningCount = stats.counts.runningCount;
    snapshot.highWatermark = stats.counts.highWatermark;

    // Return the snapshot.
    return snapshot;
}

void * MessageQueueBase::Imple::rawWaitAndGet()
{
    // Get raw memory from the raw ring buffer.
    void * pRaw = rawRingBuffer.get();

    // Manage running count and high watermark.
    ///@todo Couldn't we leverage the rawRingBuffer for this information?
    InternalRunningStateStats runningStats;
    InternalRunningStateStats runningStatsNew;
    runningStats.state = runningState.load( std::memory_order_seq_cst );
    do
    {
        // Clone atomically captured state,
        // We will be incrementing the running count, may raise the high water mark.
        runningStatsNew.state = runningStats.state;
        if ( ++runningStatsNew.counts.runningCount > runningStats.counts.highWatermark )
            runningStatsNew.counts.highWatermark = runningStatsNew.counts.runningCount;

    } while ( !runningState.compare_exchange_weak( runningStats.state, runningStatsNew.state,
                                                   std::memory_order_seq_cst, std::memory_order_seq_cst ) );
    // Zero out Arena Memory!
    std::memset( pRaw, 0, elementSize );

    // Return Raw Block
    return pRaw;
}

void MessageQueueBase::Imple::rawPutAndNotify( void * pRaw )
{
    // Put formerly cooked memory into the cooked ring buffer.
    rawRingBuffer.put( pRaw );

    // Manage running count.
    ///@todo Couldn't we leverage the rawRingBuffer for this information?
    InternalRunningStateStats runningStats;
    InternalRunningStateStats runningStatsNew;
    runningStats.state = runningState.load( std::memory_order_seq_cst );
    do
    {
        // Clone atomically captured state,
        // We will be decrementing the running count and not touching the high watermark.
        runningStatsNew.state = runningStats.state;
        --runningStatsNew.counts.runningCount;

    } while ( !runningState.compare_exchange_weak( runningStats.state, runningStatsNew.state,
                                                   std::memory_order_seq_cst, std::memory_order_seq_cst ) );
}

void MessageQueueBase::Imple::dispatchMessage( MessageBase * pMsg )
{
    nameOfLastMessageDispatched.store( pMsg->name() );
    if ( pMutex )
    {
        std::lock_guard< Mutex > lockGuard{ *pMutex };
        pMsg->dispatch();
    }
    else
    {
        pMsg->dispatch();
    }
}

size_t MessageQueueBase::Imple::getPaddedTypeAllocSize( size_t requestedElementSize )
{
    size_t alignmentOverspill = requestedElementSize % sizeof( void * );
    return  ( alignmentOverspill != 0 ) ? requestedElementSize + sizeof( void * ) - alignmentOverspill :
            requestedElementSize;
}


MessageQueueBase::MessageQueueBase( std::size_t requestedNumElements, std::size_t requestedMaxMessageSize,
                                    bool enableDispatchLocking )
  : pImple{ new Imple{ requestedNumElements, requestedMaxMessageSize, enableDispatchLocking } }
{
}

MessageQueueBase::~MessageQueueBase()
{
    // Abort and wait a bit for blocked threads to get out of the way.
    abort();
    std::this_thread::sleep_for( std::chrono::milliseconds(100) );

    delete pImple;
}

MessageQueueBase::RunningStateStats MessageQueueBase::getRunningStateStatistics() noexcept
{
    return pImple->getRunningStateStatistics();
}

void MessageQueueBase::abort()
{
    pImple->abort();
}

void * MessageQueueBase::rawWaitAndGet()
{
    return pImple->rawWaitAndGet();
}

void MessageQueueBase::cookedPutAndNotify( MessageBase * pCooked )
{
    pImple->cookedPutAndNotify( pCooked );
}

MessageBase * MessageQueueBase::cookedWaitAndGet()
{
    return pImple->cookedWaitAndGet();
}

void MessageQueueBase::rawPutAndNotify( void * pRaw )
{
    pImple->rawPutAndNotify( pRaw );
}

MessageQueueBase::AutoDispatchLock MessageQueueBase::getAutoDispatchLock()
{
    if ( !pImple->pMutex )
        throw MessageQueueDispatchLockingDisabled(
                "MessageQueueBase::getAutoDispatchLock() - Dispatch Locking not enabled when constructed" );

    return AutoDispatchLock{ this };
}

void MessageQueueBase::dispatchMessage( MessageBase * pMsg )
{
    pImple->dispatchMessage( pMsg );
}

const char * MessageQueueBase::getNameOfLastMessageDispatched()
{
    return pImple->getNameOfLastMessageDispatched();
}

bool MessageQueueBase::isAborted() noexcept
{
    return pImple->aborted;
}

size_t MessageQueueBase::getElementSize() noexcept
{
    return pImple->elementSize;
}
