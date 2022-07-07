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
    // CookedMemoryManager ensures memory is returned to the raw pool should dispatch throw.
    auto * pM = cookedWaitAndGet();
    CookedMemoryManager cookedMemoryManager{ this, pM };

    dispatchMessage( pM );
}

void MessageQueue::getAndDispatch( WakeupCallFunctionType wakeupFunctor )
{
    // CookedMemoryManager ensures memory is returned to the raw pool should wakeupFunctor
    // or dispatch throw.
    auto * pM = cookedWaitAndGet();
    CookedMemoryManager cookedMemoryManager{ this, pM };

    wakeupFunctor();

    dispatchMessage( pM );
}

void MessageQueue::purge()
{
    // Note warning in documentation. We are assuming we are being used properly.
    // We only get the running state snapshot once and then count down to zero.
    // We do not expect to block here if used properly.
    // CookedMemoryManager returns memory to the raw pool.
    auto runningState = getRunningStateStatistics();
    for ( ; 0 != runningState.runningCount; --runningState.runningCount )
        CookedMemoryManager{ this, cookedWaitAndGet() };
}
