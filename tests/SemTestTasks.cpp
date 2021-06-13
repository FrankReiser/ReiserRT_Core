//
// Created by frank on 2/18/21.
//

#include "Semaphore.hpp"

#include "SemTestTasks.h"
#include "StartingGun.h"
#include "ReiserRT_CoreExceptions.hpp"

//#include <mutex>
//#include <condition_variable>
#include <iostream>
#include <thread>

using namespace ReiserRT::Core;
using namespace std;

void SemTakeTask::operator()(StartingGun* startingGun, ReiserRT::Core::Semaphore* theSem, unsigned int nTakes)
{
    // Wait on go condition.
    state = State::waitingForGo;
    startingGun->waitForStartingShot();
    state = State::going;

    // Do our thing
    for (unsigned int i = 0; i != nTakes; ++i)
    {
        try
        {
            theSem->take();
            ++takeCount;
        }
        catch (SemaphoreAborted&)
        {
            state = State::aborted;
            return;
        }
        catch (...)
        {
            state = State::unknownExceptionDetected;
            theSem->abort();
            return;
        }
    }
    state = State::completed;
}

const char* SemTakeTask::stateStr() const
{
    switch (state.load())
    {
        case State::constructed: return "constructed";
        case State::waitingForGo: return "waitingForGo";
        case State::going: return "going";
        case State::unknownExceptionDetected: return "unknownExceptionDetected";
        case State::aborted: return "aborted";
        case State::completed: return "completed";
        default: return "unknown";
    }
}

void SemTakeTask::outputResults(unsigned int i)
{
    cout << "SemTakeTask(" << i << ") takeCount=" << takeCount
         << ", state=" << stateStr()
         << "\n";
}

void SemGiveTask::operator()(StartingGun* startingGun, ReiserRT::Core::Semaphore* theSem, unsigned int nGives)
{
    // Wait on go condition.
    state = State::waitingForGo;
    startingGun->waitForStartingShot();
    state = State::going;
    for (unsigned int i = 0; i != nGives; ++i)
    {
        for (;;)
        {
            try
            {
                theSem->give();
                this_thread::yield();   // Do not allow any one thread to pound the give key if unbounded.
                ++giveCount;
                break;
            }
            catch (SemaphoreAborted&)
            {
                state = State::aborted;
                return;
            }
            catch (...)
            {
                state = State::unknownExceptionDetected;
                theSem->abort();
                return;
            }
        }
    }
    state = State::completed;
}

const char* SemGiveTask::stateStr() const
{
    switch (state.load())
    {
        case State::constructed: return "constructed";
        case State::waitingForGo: return "waitingForGo";
        case State::going: return "going";
        case State::unknownExceptionDetected: return "unknownExceptionDetected";
        case State::aborted: return "aborted";
        case State::completed: return "completed";
        default: return "unknown";
    }
}

void SemGiveTask::outputResults(unsigned int i)
{
    cout << "SemGiveTask(" << i << ") giveCount=" << giveCount
         << ", state=" << stateStr()
         << "\n";
}

