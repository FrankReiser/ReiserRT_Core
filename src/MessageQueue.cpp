/**
* @file MessageQueue.cpp
* @brief The Specification for MessageQueue
*
* This file exists to keep the CMake suite of tools happy. Particularly certain ctest features
*
* @authors: Frank Reiser
* @date Created on Feb 17, 2021
*/

#include "MessageQueue.hpp"

#ifdef REISER_RT_HAS_PTHREADS
#include "PriorityInheritMutex.hpp"
#endif
#include <mutex>


using namespace ReiserRT;
using namespace ReiserRT::Core;

class MessageQueue::Details
{
    friend class MessageQueue;
    friend class MessageQueue::AutoDispatchLock;

#ifdef REISER_RT_HAS_PTHREADS
    using MutexType = PriorityInheritMutex;
#else
    using MutexType = std::mutex;
#endif
    Details() = default;
    ~Details() = default;

    MutexType mutex{};
};

MessageQueue::AutoDispatchLock::AutoDispatchLock( Details * pTheDetails )
  : pDetails{ pTheDetails }
{
    pDetails->mutex.lock();
}
MessageQueue::AutoDispatchLock::~AutoDispatchLock()
{
    pDetails->mutex.unlock();
}

const char * MessageBase::name() const
{
    return "Unforgiven";
}

MessageQueue::MessageQueue( size_t requestedNumElements, size_t requestedMaxMessageSize )
  : pDetails{ new Details }
  , objectPool{ requestedNumElements, requestedMaxMessageSize }
  , objectQueue{ requestedNumElements }
  , nameOfLastMessageDispatched{ nullptr }
{
}

MessageQueue::~MessageQueue()
{
    objectQueue.abort();
};

void MessageQueue::getAndDispatch()
{
#if 0
    MessagePtrType msgPtr = std::move( objectQueue.get() );
    std::lock_guard< Details::MutexType > lockGuard{ pDetails->mutex };

    nameOfLastMessageDispatched = msgPtr->name();
    msgPtr->dispatch();
#else
    // Setup a lambda function for invoking the message dispatch operation.
    auto funk = [ this ]( MessagePtrType & msgPtr )
    {
        std::lock_guard< Details::MutexType > lockGuard{ pDetails->mutex };
        nameOfLastMessageDispatched = msgPtr->name();
        msgPtr->dispatch();
    };

    // Get (wait) for a message and invoke our lambda via reference.
    // If the dispatch throws anything, it will propagate up the call stack.
    objectQueue.getAndInvoke( std::ref( funk ) );
#endif
}

void MessageQueue::getAndDispatch( WakeupCallFunctionType wakeupFunctor )
{
#if 0
    MessagePtrType msgPtr = std::move( objectQueue.get() );
    std::lock_guard< Details::MutexType > lockGuard{ pDetails->mutex };

    wakeupFunctor();
    nameOfLastMessageDispatched = msgPtr->name();
    msgPtr->dispatch();
#else
    // Setup a lambda function for invoking the message dispatch operation.
    auto funk = [ this, &wakeupFunctor ]( MessagePtrType & msgPtr )
    {
        // Call the wakeup function first before we take the lock.
        // It's primary usage is to record processing time and we do not desire this next
        // potential blocking on lock acquisition to not be counted in those statistics.
        wakeupFunctor();
        std::lock_guard< Details::MutexType > lockGuard{ pDetails->mutex };
        nameOfLastMessageDispatched = msgPtr->name();
        msgPtr->dispatch();
    };

    // Get (wait) for a message and invoke our lambda via reference.
    // If the dispatch throws anything, it will propagate up the call stack.
    objectQueue.getAndInvoke( std::ref( funk ) );
#endif
}

const char * MessageQueue::getNameOfLastMessageDispatched()
{
    return nameOfLastMessageDispatched;
}

void MessageQueue::abort()
{
    objectQueue.abort();
}

MessageQueue::RunningStateStats MessageQueue::getRunningStateStatistics() noexcept
{
    return objectQueue.getRunningStateStatistics();
}

MessageQueue::AutoDispatchLock MessageQueue::getAutoDispatchLock()
{
    AutoDispatchLock autoDispatchLock{ pDetails };
    return std::move( autoDispatchLock );
}
