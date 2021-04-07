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

MessageQueue::MessageQueue( size_t requestedNumElements, size_t requestedMaxMessageSize, bool enableDispatchLocking )
  : MessageQueueBase( requestedNumElements, requestedMaxMessageSize, enableDispatchLocking )
{
}

void MessageQueue::getAndDispatch()
{
    auto * pM = cookedWaitAndGet();

    CookedMemoryManager cookedMemoryManager{ this, pM };

    dispatchMessage( pM );
}

void MessageQueue::getAndDispatch( WakeupCallFunctionType wakeupFunctor )
{
    auto * pM = cookedWaitAndGet();

    wakeupFunctor();

    CookedMemoryManager cookedMemoryManager{ this, pM };

    dispatchMessage( pM );
}
