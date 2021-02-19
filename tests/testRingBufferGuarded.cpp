//
// Created by frank on 2/19/21.
//

#include "RingBufferGuarded.hpp"

#include "RingBufferGuardedTestTasks.h"
#include "StartingGun.h"

#include <random>
#include <memory>
#include <vector>
#include <thread>
#include <iostream>

using namespace std;
using namespace ReiserRT::Core;

int main()
{
    int retVal = 0;

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

            uniform_int_distribution< unsigned int > uniformDistributionRB;
            default_random_engine randEngineRB;

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

            // Instantiate the put and get task threads
            StartingGun startingGun;
            for (unsigned int i = 0; i != numCores; ++i)
            {
                putThreads[i].reset(new thread{ ref(putTasks[i]), &startingGun, &ringBuffer, testDataVec[i].get(), queueSize });
                getThreads[i].reset(new thread{ ref(getTasks[i]), &startingGun, &ringBuffer, queueSize });
            }

            // Let her rip!
            startingGun.pullTrigger();

            // Join threads
            for (unsigned int i = 0; i != numCores; ++i)
            {
                putThreads[i]->join();
                getThreads[i]->join();
            }

            // Check Stats
            bool incomplete = false;
            for (unsigned int i = 0; i != numCores; ++i)
            {
                if (putTasks[i].getState() != PutTaskRBG::State::completed)
                {
                    cout << " Task putTask[" << i << "] did not complete!\n";
                    incomplete = true;
                }
                if (getTasks[i].getState() != GetTaskRBG::State::completed)
                {
                    cout << " Task getTask[" << i << "] did not complete!\n";
                    incomplete = true;
                }
            }
            if ( incomplete ) {
                retVal = 1;
                break;
            }

            // Check that each piece of data was accessed numCores times
            for (unsigned int i = 0; !incomplete && i != numCores; ++i)
            {
                const ThreadTestDataRBG * pTestData = testDataVec[i].get();
                for (unsigned int j = 0; !incomplete  && j != queueSize; ++j)
                {
                    unsigned int validatedInvocations = pTestData[j].getValidatedInvocations();
                    if ( 1 != validatedInvocations )
                    {
                        cout << " testData[" << i << "][" << j << "] not accessed one time, got " << validatedInvocations << ".\n";
                        incomplete = true;

                    }
                }
            }
            if ( incomplete ) {
                // Task States
                for (unsigned int i = 0; i != numCores; ++i)
                {
                    putTasks[i].outputResults(i);
                    getTasks[i].outputResults(i);
                }
                retVal = 2;
                break;
            }
        }

    } while ( false );

    return retVal;
}
