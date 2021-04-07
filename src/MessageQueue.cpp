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
#include <atomic>

using namespace ReiserRT;
using namespace ReiserRT::Core;

#if 0
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
    * @brief Mutex Pointer Type.
    *
    * We encapsulate our MutexType as a unique_ptr as we may or may not instantiate it.
    * and if we do, it will take care of itself.
    */
    using MutexPtrType = std::unique_ptr< MutexType >;

    /**
    * @brief Constructor
    *
    * This Constructor will instantiate a mutex on the heap if we are enabling dispatch locking.
    * Otherwise, we won't pay for it. The pointer will be used as a flag as to whether or not we instantiated it.
    */
    Details( bool enableDispatchLocking ) : pMutex{ enableDispatchLocking ? new MutexType{} : nullptr } {}

    /**
    * @brief Destructor
    *
    * This Destructor is defaulted.
    */
    ~Details() = default;

    /**
    * @brief Get Name of Last Message Dispatched
    *
    * This operation returns the name of the last message dispatched in a thread safe way.
    *
    * @return Returns the name of the last message dispatched.
    */
    const char * getNameOfLastMessageDispatched()
    {
        return nameOfLastMessageDispatched.load();
    }

    /**
    * @brief Dispatch a Message
    *
    * This wrapper operation stores the name of the message prior to dispatch. We do this in the case
    * that should the operation throw, the name that threw is recorded.
    * If dispatch locking has been enabled, we will lock the dispatch mutex just prior to dispatching the message.
    *
    * @param msgPtr
    */
    void dispatchMessage( MessagePtrType & msgPtr )
    {
        nameOfLastMessageDispatched.store( msgPtr->name() );
        if ( pMutex )
        {
            std::lock_guard< Details::MutexType > lockGuard{ *pMutex };
            msgPtr->dispatch();
        }
        else
        {
            msgPtr->dispatch();
        }
    }

    /**
    * @brief Mutex Instance
    *
    * Our mutex instance pointer and flag. If it is a nullptr, then dispatch locking is disabled and we
    * do not use it. Otherwise, we use it to lock while dispatching.
    */
    MutexPtrType pMutex;

    /**
    * @brief The Name of the Last Message Dispatched
    *
    * This attribute maintains the name of the last message dispatched by the MessageQueue.
    * We use an atomic pointer so that it can be fetched in a thread safe manner.
    */
    std::atomic< const char * > nameOfLastMessageDispatched{ "[NONE]" };
};

MessageQueue::AutoDispatchLock::AutoDispatchLock( Details * pTheDetails )
  : pDetails{ pTheDetails }
{
    // We shall not construct AutoDispatchLock with a null mutex pointer.
    pDetails->pMutex->lock();
}
MessageQueue::AutoDispatchLock::~AutoDispatchLock()
{
    if ( pDetails )
        pDetails->pMutex->unlock();
}
#endif
#if 0
const char * MessageBase::name() const
{
    return "Unforgiven";
}
#endif

MessageQueue::MessageQueue( size_t requestedNumElements, size_t requestedMaxMessageSize, bool enableDispatchLocking )
#if 1
  : MessageQueueBase( requestedNumElements, requestedMaxMessageSize, enableDispatchLocking )
#else
  : pDetails{ new Details{ enableDispatchLocking } }
  , objectPool{ requestedNumElements, requestedMaxMessageSize }
  , objectQueue{ requestedNumElements }
#endif
{
}

MessageQueue::~MessageQueue()
{
#if 1

#else
    objectQueue.abort();
#endif
};

void MessageQueue::getAndDispatch()
{
#if 1
    MessageBase * pM = reinterpret_cast< MessageBase * >( cookedWaitAndGet() );

    CookedMemoryManager cookedMemoryManager{ this, pM };

    dispatchMessage( pM );

#else
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
        pDetails->dispatchMessage( msgPtr );
    };

    // Get (wait) for a message and invoke our lambda via reference.
    // If the dispatch throws anything, it will propagate up the call stack.
    objectQueue.getAndInvoke( std::ref( funk ) );
#endif
}

void MessageQueue::getAndDispatch( WakeupCallFunctionType wakeupFunctor )
{
#if 1
    MessageBase * pM = reinterpret_cast< MessageBase * >( cookedWaitAndGet() );

    wakeupFunctor();

    CookedMemoryManager cookedMemoryManager{ this, pM };

    dispatchMessage( pM );
#else
    ///@note See note for getAndDispatch without the wakeupFunctor argument for discussion

    // Setup a lambda function for invoking the message dispatch operation.
    auto funk = [ this, &wakeupFunctor ]( MessagePtrType & msgPtr )
    {
        // Call the wakeup function first before we take the lock.
        // It's primary usage is to record processing time and we do not desire this next
        // potential blocking on lock acquisition to not be counted in those statistics.
        wakeupFunctor();
        pDetails->dispatchMessage( msgPtr );
    };

    // Get (wait) for a message and invoke our lambda via reference.
    // If the dispatch throws anything, it will propagate up the call stack.
    objectQueue.getAndInvoke( std::ref( funk ) );
#endif
}

#if 0
const char * MessageQueue::getNameOfLastMessageDispatched()
{
    return pDetails->getNameOfLastMessageDispatched();
}
#endif

#if 0
void MessageQueue::abort()
{
    objectQueue.abort();
}
#endif

#if 0
MessageQueueBase::RunningStateStats MessageQueue::getRunningStateStatistics() noexcept
{
    return objectQueue.getRunningStateStatistics();
}
#endif

#if 0
MessageQueue::AutoDispatchLock MessageQueue::getAutoDispatchLock()
{
    if ( !pDetails->pMutex )
        throw std::runtime_error( "MessageQueue::getAutoDispatchLock() - Dispatch Locking not enabled when constructed" );

    return std::move( AutoDispatchLock{ pDetails } );
}
#endif
