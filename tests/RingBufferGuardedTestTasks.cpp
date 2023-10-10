//
// Created by frank on 2/19/21.
//

#include "RingBufferGuardedTestTasks.h"
#include "StartingGun.h"

#include <random>
#include <iostream>

using namespace std;
using namespace ReiserRT::Core;

uniform_int_distribution< unsigned int > uniformDistributionRB;
default_random_engine randEngineRB{ std::random_device{}() };


ThreadTestDataRBG::ThreadTestDataRBG() : randNum{uniformDistributionRB(randEngineRB)}, randNumHash{munger(randNum)}
{
}

void
PutTaskRBG::operator()(StartingGun* startingGun, RingBufferGuarded< const ThreadTestDataRBG* >* theRing,
        const ThreadTestDataRBG* testData, unsigned int nElements) {
    // Wait on go condition.
    state = State::waitingForGo;
    startingGun->waitForStartingShot();
    state = State::going;

    // Do our thing
    for (unsigned int i = 0; i != nElements; ++i) {
        for (;;) {
            // If aborted, set state and return
            if (startingGun->isAborted())
            {
                state = State::aborted;
                return;
            }

            // We should never catch an exception if it is designed correctly.
            try
            {
                // If no throw, the data was successfully "put", break out of inner loop.
                theRing->put(testData + i);
                ++completionCount;

                // Spin a random quantity before breakout to soften the attack.
                unsigned int spin = testData[i].getRandNum() & 0x1FF;
                for (unsigned int j = 0; j != spin; ++j)
                    ;

                break;
            }
            catch (const RingBufferOverflow&)
            {
                state = State::overflowDetected;
                return;
            }
            catch (...)
            {
                state = State::unknownExceptionDetected;
                startingGun->abort();
                return;
            }
        }
    }
    state = State::completed;
}

void PutTaskRBG::outputResults(unsigned int i) const
{
    cout << "PutTaskRB(" << i << ") completionCount=" << completionCount
         << ", state=" << stateStr()
         << endl;
}

void GetTaskRBG::operator()(StartingGun* startingGun, RingBufferGuarded< const ThreadTestDataRBG* >* theRing,
                unsigned int nElements) {
    // Wait on go condition.
    state = State::waitingForGo;
    startingGun->waitForStartingShot();
    state = State::going;

    // Do our thing
    for (unsigned int i = 0; i != nElements; ++i) {
        for (;;) {
            // If aborted, set state and return
            if (startingGun->isAborted()) {
                state = State::aborted;
                return;
            }

            // We should never catch an exception if it is designed correctly.
            try {
                // If no throw, the data was successfully "put", break out of inner loop.
                const ThreadTestDataRBG* testData = theRing->get();
                if (testData == nullptr)
                {
                    state = State::nullDataDetected;
                    startingGun->abort();
                    return;
                }
                if (!testData->isValid())
                {
                    state = State::invalidDataDetected;
                    startingGun->abort();
                    return;
                }
                ++completionCount;

                // Spin a random quantity before breakout to soften the attack.
                unsigned int spin = testData->getRandNum() & 0x1FF;
                for (unsigned int j = 0; j != spin; ++j)
                    ;

                break;
            }
            catch (...)
            {
                state = State::unknownExceptionDetected;
                startingGun->abort();
                return;
            }
        }
    }
    state = State::completed;
}

void GetTaskRBG::outputResults(unsigned int i) const
{
    cout << "GetTaskRB(" << i << ") completionCount=" << completionCount
         << ", state=" << stateStr()
         << endl;
}

