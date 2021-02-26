//
// Created by frank on 2/19/21.
//

#include "RingBufferGuarded.hpp"

#include "RingBufferGuardedTestTasks.h"
#include "StartingGun.h"

#include <memory>
#include <vector>
#include <thread>
#include <iostream>

using namespace std;
using namespace ReiserRT::Core;

int main()
{
    auto retVal = 0;

    do {
        // Multi-threaded contention testing.
        ///@note Our Ring Buffer Simple and our Semaphore implementations have been fairly well tested.
        ///We only need to verify that we provide the necessary synchronization against concurrent access for our
        ///guarded ring buffer.
        {
            // Fake this as 8. The standard insists on a constexpr for array dimension.
            // So I can't initialize it at runtime with hardware_concurrency call.
            constexpr unsigned int numCores = 8;

            // Our Ring Buffer Size
            constexpr unsigned int queueSize = 262144;
//            constexpr unsigned int queueSize = 262144 >> 2;

            // Allocate our test data. We will use the queueSize here as that, multiplied by numerous put threads,
            // will exceed the ringBuffer capacity, but get threads will also be running getting data out of
            // the ring buffer.  There will most certainly be some exceptions thrown, but we can handle that.
            using TestDataPtrPtr = unique_ptr< const ThreadTestDataRBG[] >;
            using TestDataVect = vector< TestDataPtrPtr >;

            TestDataVect testDataVec;
            testDataVec.reserve( numCores );
            for (unsigned int i = 0; i != numCores; ++i)
            {
                testDataVec.emplace_back(new ThreadTestDataRBG[queueSize]);
            }

            // The RingBuffer
            RingBufferGuarded< const ThreadTestDataRBG* > ringBuffer(queueSize);

            // Put and Get Task Functors, which can return state information when the runs are complete.
            PutTaskRBG putTasks[numCores];
            GetTaskRBG getTasks[numCores];

            // Put and Get Threads
            unique_ptr< thread >  putThreads[numCores];
            unique_ptr< thread > getThreads[numCores];

            // Instantiate the starting gun
            StartingGun startingGun;

            // Spawn the get task threads
//            cout << "Instantiating Get Tasks" << endl;
            for (unsigned int i = 0; i != numCores; ++i)
            {
                getThreads[i].reset(new thread{ ref(getTasks[i]), &startingGun, &ringBuffer, queueSize });
            }

            // Spawn the put task threads
//            cout << "Instantiating Put Tasks" << endl;
            for (unsigned int i = 0; i != numCores; ++i)
            {
                putThreads[i].reset(new thread{ ref(putTasks[i]), &startingGun, &ringBuffer, testDataVec[i].get(), queueSize });
            }

            // We have launched a bunch of threads. We should not short circuit our joining of them.
            // This do once structure helps ensure that we do not fail to join each thread should we bail out.
            do {
                // Wait for the get tasks to achieve the waitingForGo state.  This should be be immediate.
                unsigned int n = 0;
                for (; n != 10; ++n)
                {
                    unsigned int i = 0;
                    for (; i != numCores; ++i)
                    {
                        if (getTasks[i].getState() != GetTaskRBG::State::waitingForGo) {
                            this_thread::sleep_for(chrono::milliseconds(10));
                            break;
                        }
                    }
                    if (numCores==i)
                    {
                        break;
                    }
                }
                if (n == 10)
                {
                    cout << "GetTaskRBG failed to reach the waitingForGo state!\n";
                    retVal = 1;
                    break;
                }

                // Wait for the put tasks to get to the waitingForGo state.  This should be relatively quick.
                for (n = 0; n != 10; ++n)
                {
                    unsigned int i = 0;
                    for (; i != numCores; ++i)
                    {
                        if (putTasks[i].getState() != PutTaskRBG::State::waitingForGo) {
                            this_thread::sleep_for(chrono::milliseconds(10));
                            break;
                        }
                    }
                    if (numCores==i)
                    {
                        break;
                    }
                }
                if (n == 10)
                {
                    cout << "PutTaskRBG failed to reach the waitingForGo state!!!!\n";
                    retVal = 2;
                    break;
                }
//                cout << "All tasks waiting for starting gun" << endl;

                // Let her rip
                startingGun.pullTrigger();

                // Wait for all the get tasks to get to the going state.  This should be relatively quick.
                for (n = 0; n != 10; ++n)
                {
                    unsigned int i = 0;
                    for (; i != numCores; ++i)
                    {
                        if (getTasks[i].getState() != GetTaskRBG::State::going) {
                            this_thread::sleep_for(chrono::milliseconds(10));
                            break;
                        }
                    }
                    if (numCores==i)
                    {
                        break;
                    }
                }
                if (n == 10)
                {
                    cout << "GetTaskRBG failed to reach the going state!\n";
                    retVal = 3;
                    break;
                }

                // Wait for all the put tasks to get to the going state.  This should be relatively quick.
                for (n = 0; n != 10; ++n)
                {
                    unsigned int i = 0;
                    for (; i != numCores; ++i)
                    {
                        if (putTasks[i].getState() != PutTaskRBG::State::going) {
                            this_thread::sleep_for(chrono::milliseconds(10));
                            break;
                        }
                    }
                    if (numCores==i)
                    {
                        break;
                    }
                }
                if (n == 10)
                {
                    cout << "PutTaskRBG failed to reach the going state!\n";
                    retVal = 4;
                    break;
                }
//                cout << "All tasks GOING" << endl;

                // Threads should be putting into and getting from the guarded ring buffer.
                // We will monitor the get tasks as they are the last in line.
                // They should be in either the completed or going state. Any other state
                // is a problem.
                constexpr int maxLoops = 400;
                bool failed = false;
                for (n = 0; n != maxLoops; ++n) {
                    this_thread::sleep_for(chrono::milliseconds(1000));
                    unsigned int numCompleted = 0;
                    for (unsigned int j = 0; j != numCores; ++j) {
                        GetTaskRBG::State gState = getTasks[j].getState();
                        if (GetTaskRBG::State::going != gState && GetTaskRBG::State::completed != gState) {
                            cout << "Ill state of \"" << getTasks[j].stateStr()
                                << "\" detected for getTasks[" << j << "]" << endl;
                            failed = true;
                            break;
                        }
                        if ( gState == GetTaskRBG::State::completed ) ++numCompleted;
                    }
                    // If we failed or numCompleted is the number of get threads we've spun, then all is done
                    if ( failed || numCores == numCompleted )
                        break;
                }

                // If failed set return value and break out.
                if ( failed ) {
                    cout << "FAILED Multi-threaded contention testing" << endl;
                    retVal = 5;
                    break;
                }

                // Obviously, the PutTasks should all be completed too. That would be a sanity check of the test itself.

                // Check that each piece of data was accessed and validated, numCores times
                bool invalid = false;
                for (unsigned int i = 0; !invalid && i != numCores; ++i)
                {
                    const ThreadTestDataRBG * pTestData = testDataVec[i].get();
                    for (unsigned int j = 0; !invalid && j != queueSize; ++j)
                    {
                        unsigned int validatedInvocations = pTestData[j].getValidatedInvocations();
                        if ( 1 != validatedInvocations )
                        {
                            cout << " testData[" << i << "][" << j << "] not accessed one time, got " << validatedInvocations << ".\n";
                            invalid = true;

                        }
                    }
                }
                if ( invalid ) {
                    // Task States
                    for (unsigned int i = 0; i != numCores; ++i)
                    {
                        cout << "Transmission Validation Check FAILED!" << endl;
                        putTasks[i].outputResults(i);
                        getTasks[i].outputResults(i);
                    }
                    retVal = 6;
                    break;
                }


            } while (false);

            // Join threads
            startingGun.abort(); // For good measure in case a thread failed to get going or is otherwise stuck.
            ringBuffer.abort();  // Ditto.
            for (unsigned int i = 0; i != numCores; ++i)
            {
                putThreads[i]->join();
                getThreads[i]->join();
            }

            // If we failed, provide information on the state of things.
            if ( 0 != retVal ) {
                // Task States
                for (unsigned int i = 0; i != numCores; ++i)
                {
                    putTasks[i].outputResults(i);
                    getTasks[i].outputResults(i);
                }
                break;
            }

        }

    } while ( false );

    return retVal;
}
