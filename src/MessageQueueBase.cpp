/**
* @file MessageQueueBase.cpp
* @brief The Implementation file for MessageQueueBase
*
* This file came into existence in a effort to replace and ObjectPool usage and ObjectQueue implementation
* in an effort to eliminate an extra Mutex take/release on enqueueing and de-queueing of messages.
* ObjectQueue will be eliminated in this process.
*
* @authors Frank Reiser
* @date Created on Apr 6, 2015
*/

#include "MessageQueueBase.hpp"
#include "RingBufferGuarded.hpp"

#include <cstring>
#include <thread>

using namespace ReiserRT::Core;

const char * MessageBase::name() const
{
    return "Unforgiven";
}
class ReiserRT_Core_EXPORT MessageQueueBase::Imple
{
private:
    friend class MessageQueueBase;

    using RingBufferType = RingBufferGuarded< void * >;

    using RunningStateBasis = uint64_t;
    using RunningStateType = std::atomic< RunningStateBasis >;
    union InternalRunningStateStats
    {
        struct
        {
            CounterType runningCount;    //!< The Current Running Count Captured Atomically (snapshot)
            CounterType highWatermark;   //!< The Current High Watermark Captured Atomically (snapshot)
        };

        RunningStateBasis state{ 0 };
    };

    Imple( std::size_t theRequestedNumElements, std::size_t theElementSize, bool enableDispatchLocking );

    ~Imple();

    RunningStateStats getRunningStateStatistics() noexcept;

    void abort()
    {
        aborted = true;
        cookedRingBuffer.abort();
        rawRingBuffer.abort();
    }

    void * rawWaitAndGet();

    inline void cookedPutAndNotify( void * pCooked )
    {
        // Put cooked memory into the cooked ring buffer. The guarded ring buffer performs the notification.
        cookedRingBuffer.put( pCooked );
    }

    inline void * cookedWaitAndGet()
    {
        // Get cooked memory from the cooked ring buffer. The guarded ring buffer performs the wait.
        return cookedRingBuffer.get();
    }

    void rawPutAndNotify( void * pRaw );

    void dispatchMessage( MessageBase * pMsg );

    const char * getNameOfLastMessageDispatched()
    {
        return nameOfLastMessageDispatched.load();
    }

    inline void flush( MessageQueueBase::FlushingFunctionType & operation )
    {
        auto funk = [ operation ]( void * pV ) noexcept { operation( pV ); };
        cookedRingBuffer.flush( std::ref( funk ) );
    }

    const size_t requestedNumElements;
    const size_t elementSize;

    unsigned char * arena;
    std::atomic< const char * > nameOfLastMessageDispatched{ "[NONE]" };

    RingBufferType rawRingBuffer;
    RingBufferType cookedRingBuffer;

    RunningStateType runningState{};

    std::atomic_bool aborted{ false };

};

MessageQueueBase::Imple::Imple( std::size_t theRequestedNumElements, std::size_t theElementSize,
                                bool enableDispatchLocking )
  : requestedNumElements{ theRequestedNumElements }
  , elementSize{ theElementSize }
  , arena{ new unsigned char [ theElementSize * requestedNumElements ] }
  , rawRingBuffer{ theRequestedNumElements, true }
  , cookedRingBuffer{ theRequestedNumElements }
{
    // Prime the raw ring buffer with void pointers into our arena space. We do this with a lambda function.
    auto funk = [ this, theElementSize ]( size_t i ) noexcept { return reinterpret_cast< void * >( arena + i * theElementSize ); };
    rawRingBuffer.prime( std::ref( funk ) );
}

MessageQueueBase::Imple::~Imple()
{
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
    snapshot.runningCount = stats.runningCount;
    snapshot.highWatermark = stats.highWatermark;

    // Return the snapshot.
    return snapshot;
}

void * MessageQueueBase::Imple::rawWaitAndGet()
{
    // Get raw memory from the raw ring buffer. The guarded ring buffer performs the wait.
    void * pRaw = rawRingBuffer.get();

    // Manage running count and high water mark.
    ///@todo Shouldn't we leverage the rawRingBuffer for this information.
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
    // Zero out Arena Memory!
    std::memset( pRaw, 0, elementSize );

    // Return Raw Block
    return pRaw;
}

void MessageQueueBase::Imple::rawPutAndNotify( void * pRaw )
{
    // Put formerly cooked memory into the cooked ring buffer. The guarded ring buffer performs the notify.
    rawRingBuffer.put( pRaw );

    // Manage running count.
    ///@todo Shouldn't we leverage the rawRingBuffer for this information.
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

void MessageQueueBase::Imple::dispatchMessage( MessageBase * pMsg )
{
    nameOfLastMessageDispatched.store( pMsg->name() );
    pMsg->dispatch();
}

MessageQueueBase::MessageQueueBase( std::size_t requestedNumElements, std::size_t elementSize,
                                    bool enableDispatchLocking )
  : pImple{ new Imple{ requestedNumElements, elementSize, enableDispatchLocking } }
{
}

MessageQueueBase::~MessageQueueBase()
{
    abort();
    std::this_thread::sleep_for( std::chrono::milliseconds(100) );

    auto funk = []( void * pV ) noexcept { MessageBase * pM = reinterpret_cast< MessageBase * >( pV ); pM->~MessageBase(); };
    flush( std::ref( funk ) );

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

void MessageQueueBase::cookedPutAndNotify( void * pCooked )
{
    pImple->cookedPutAndNotify( pCooked );
}

void * MessageQueueBase::cookedWaitAndGet()
{
    return pImple->cookedWaitAndGet();
}

void MessageQueueBase::rawPutAndNotify( void * pRaw )
{
    pImple->rawPutAndNotify( pRaw );
}

void MessageQueueBase::dispatchMessage( MessageBase * pMsg )
{
    pImple->dispatchMessage( pMsg );
}

const char * MessageQueueBase::getNameOfLastMessageDispatched()
{
    return pImple->getNameOfLastMessageDispatched();
}

void MessageQueueBase::flush( FlushingFunctionType operation )
{
    pImple->flush( operation );
}

bool MessageQueueBase::isAborted() noexcept
{
    return pImple->aborted;
}
