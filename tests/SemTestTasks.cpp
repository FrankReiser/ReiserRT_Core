//
// Created by frank on 2/18/21.
//

#include "SemTestTasks.h"

#include "Semaphore.hpp"
#include "StartingGun.h"

#include <mutex>
#include <condition_variable>
#include <iostream>

using namespace ReiserRT::Core;
using namespace std;

void SemTakeTask2::operator()(StartingGun* startingGun, ReiserRT::Core::Semaphore* theSem, unsigned int nTakes)
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
            theSem->wait();
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
            theSem->abort();
            return;
        }
    }
    state = State::completed;
}

const char* SemTakeTask2::stateStr() const
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

void SemTakeTask2::outputResults(unsigned int i)
{
    cout << "SemTakeTask(" << i << ") takeCount=" << takeCount
         << ", state=" << stateStr()
         << "\n";
}

void SemGiveTask2::operator()(StartingGun* startingGun, ReiserRT::Core::Semaphore* theSem, unsigned int nGives)
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
                theSem->notify();
                ++giveCount;
                break;
            }
            catch (overflow_error&)   // Should never happen in this test.
            {
                state = State::overflowDetected;
                theSem->abort();
                return;
            }
            catch (runtime_error&)
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

const char* SemGiveTask2::stateStr() const
{
    switch (state.load())
    {
        case State::constructed: return "constructed";
        case State::waitingForGo: return "waitingForGo";
        case State::going: return "going";
        case State::unknownExceptionDetected: return "unknownExceptionDetected";
        case State::aborted: return "aborted";
        case State::overflowDetected: return "overflowDetected";
        case State::completed: return "completed";
        default: return "unknown";
    }
}

void SemGiveTask2::outputResults(unsigned int i)
{
    cout << "SemGiveTask(" << i << ") giveCount=" << giveCount
         << ", state=" << stateStr()
         << "\n";
}

