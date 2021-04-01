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
    if ( pDetails )
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
    ///@note What is happening here, for numerous reasons, is as follows:
    ///1) MessagePtrType is a unique_ptr type, That is what comes from our objectQueue.
    ///   It should also be noted that ObjectQueue was specifically designed for MessageQueue,
    ///2) We cannot use objectPool::get to return this value as our put and emplace operations
    ///   will unblock and and attempt to create objects from ObjectPool and we would be holding
    ///   the goods. This has resulted in underflow being thrown during stress testing.
    ///Reason: ObjectPool and ObjectQueue are typically the same size for a MessageQueue.
    ///ObjectPool's only guard against underflow is ObjectQueue's blocking. We cannot
    ///allow this to unblock until MsgPtrType has been destroyed.
    ///Therefore, the bottom line is, we must let ObjectQueue call us with a reference to
    ///the unique_ptr so we can do our business before ObjectQueue signals availability for put
    ///and emplace. I have lived this before and now I have lived it again. I decided to document it.

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
}

void MessageQueue::getAndDispatch( WakeupCallFunctionType wakeupFunctor )
{
    ///@note See note for getAndDispatch without the wakeupFunctor argument for discussion

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
    return std::move( AutoDispatchLock{ pDetails } );
}
