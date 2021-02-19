//
// Created by frank on 2/18/21.
//

#include "semTakeTasks.h"

#include "Semaphore.hpp"

#include <mutex>
#include <condition_variable>

using namespace ReiserRT::Core;
using namespace std;

void SemTakeTask2::operator()(ReiserRT::Core::Semaphore & theSem, unsigned int nTakes)
{
    // Wait on go condition.
    state = State::waitingForGo;
    {
        unique_lock<mutex> lock(goConditionMutexS);
        goConditionS.wait(lock, [] { return goFlagS.load(); });
    }
    state = State::going;
    for (unsigned int i = 0; i != nTakes; ++i)
    {
        try
        {
            theSem.wait();
            ++takeCount;
        }
        catch (runtime_error&)
        {
            state = State::aborted;
            return;
        }
        catch (...)
        {
            state = State::unknownExceptionDetected;
            theSem.abort();
            return;
        }
    }
    state = State::completed;
}

