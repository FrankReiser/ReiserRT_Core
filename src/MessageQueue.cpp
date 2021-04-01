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

/**
* @brief MessageQueue Hidden Details
*
* This class contains the hidden details for MessageQueue state data.
*
*/
class MessageQueue::Details
{
    /**
    * @brief Friend Declaration
    *
    * MessageQueue needs access to construct an instance.
    */
    friend class MessageQueue;

    /**
    * @brief Friend Declaration
    *
    * MessageQueue::AutoDispatchLock needs access to our lock and unlock members.
    */
    friend class MessageQueue::AutoDispatchLock;

    /**
    * @brief Mutex Type Declaration
    *
    * The mutex type we will use.
    */
#ifdef REISER_RT_HAS_PTHREADS
    using MutexType = PriorityInheritMutex;
#else
    using MutexType = std::mutex;
#endif

    /**
    * @brief Constructor
    *
    * This Constructor is defaulted.
    */
    Details() = default;

    /**
    * @brief Destructor
    *
    * This Destructor is defaulted.
    */
    ~Details() = default;

    /**
    * @brief Mutex Instance
    *
    * Our mutex instance.
    */
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
