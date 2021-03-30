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

using namespace ReiserRT;
using namespace ReiserRT::Core;


const char * MessageBase::name() const
{
    return "Unforgiven";
}

MessageQueue::MessageQueue( size_t requestedNumElements, size_t requestedMaxMessageSize )
  : objectPool{ requestedNumElements, requestedMaxMessageSize }
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
    // Setup a lambda function for invoking the message dispatch operation.
    auto funk = [ this ]( MessagePtrType & msgPtr )
    {
        nameOfLastMessageDispatched = msgPtr->name();
        msgPtr->dispatch();
    };

    // Get (wait) for a message and invoke our lambda via reference.
    // If the dispatch throws anything, it will propagate up the call stack.
    objectQueue.getAndInvoke( std::ref( funk ) );
}

void MessageQueue::getAndDispatch( WakeupCallFunctionType wakeupFunctor )
{
    // Setup a lambda function for invoking the message dispatch operation.
    auto funk = [ this, &wakeupFunctor ]( MessagePtrType & msgPtr )
    {
        wakeupFunctor();
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
