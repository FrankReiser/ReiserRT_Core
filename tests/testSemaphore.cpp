//
// Created by frank on 2/18/21.
//

// What we are testing
#include "Semaphore.hpp"

// Test task class specifications for give and taking the semaphore.
#include "SemTestTasks.h"
#include "StartingGun.h"

// Standard stuff
#include <iostream>
#include <memory>
#include <thread>

using namespace ReiserRT::Core;
using namespace std;

// Evidently, this function is too complex for "context-sensitive data flow analysis".
// Perhaps it is the high multi-threaded nature of it. However, I am not certain about that. I turned it off.
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCDFAInspection"
int main() {
    auto retVal = 0;

    do {
        // Construction, no-wait take testing.
        // We will not take or give within this block as that could hang the test if
        // semaphore is defective. We will put those potential issues off on to other tasks later
        // on down.
        {
            Semaphore sem{ 4 };
            if (sem.getAvailableCount() != 4)
            {
                cout << "Semaphore should have reported an available count of 4 and reported " << sem.getAvailableCount() << "!" << endl;
                retVal = 1;
                break;
            }
        }

        // Test the new functionality of take with function object where the available count is restored if an exception
        // is thrown by the user provided function object.
        {
            Semaphore sem{ 4 };

            // Call take with callback functor.
            {
                bool failed = true;
                auto funk = [](){ throw std::exception{}; };
                try {
                    sem.take(std::ref(funk));
                }
                catch( std::exception &) {
                    failed = false;
                }
                if ( failed ) {
                    cout << "Expected an exception!" << endl;
                    retVal = 2;
                    break;
                }
                else {
                    // The available count should still be 4.
                    if (sem.getAvailableCount() != 4)
                    {
                        cout << "Semaphore should have reported an available count of 4 when callback throws and reported "
                        << sem.getAvailableCount() << "!" << endl;
                        retVal = 3;
                        break;
                    }
                }
            }

            // Repeat test with a callback that does not throw
            {
                size_t callbackCount = 0;
                auto funk = [&callbackCount]() { ++callbackCount; };
                sem.take(std::ref(funk));

                // Verify the callback was invoked.
                if (1 != callbackCount)
                {
                    cout << "Semaphore callback not invoked!" << endl;
                    retVal = 4;
                    break;
                }

                // The available count should now be three.
                if (sem.getAvailableCount() != 3)
                {
                    cout << "Semaphore should have reported an available count of 3 after take and reported " << sem.getAvailableCount() << "!" << endl;
                    retVal = 5;
                    break;
                }

                // Perform a give
                sem.give(std::ref(funk));

                // Verify the callback was invoked
                if (2 != callbackCount)
                {
                    cout << "Semaphore callback not invoked!" << endl;
                    retVal = 6;
                    break;
                }

                // Verify that the available count returns to 4.
                if (sem.getAvailableCount() != 4)
                {
                    cout << "Semaphore should have reported an available count of 4 after give and reported " << sem.getAvailableCount() << "!" << endl;
                    retVal = 7;
                    break;
                }
            }
        }

        // Simple pending/abort testing with one take thread running alongside this thread.
        {
            Semaphore sem{0};  // Initially empty

            // Instantiate one take task functor.
            SemTakeTask takeTask;

            // Instantiate one thread object for a take thread.
            unique_ptr< thread > takeThread;

            // Instantiate the starting gun
            StartingGun startingGun;

            // Spawn take thread invoking take task functor. It should complete after 1 take.
            takeThread.reset(new thread{ ref(takeTask), &startingGun, &sem, 1 });

            // We have launched a thread. We should not short circuit our joining of it.
            // This do once structure helps ensure that we do not fail to join the thread should we bail out.
            do {
                // Wait for it to get to the waitingForGo state.  This should be relatively quick.
                unsigned int n = 0;
                for (; n != 10; ++n)
                {
                    if (takeTask.getState() == SemTakeTask::State::waitingForGo) break;
                    this_thread::sleep_for(chrono::milliseconds(10));
                }
                if (n == 10)
                {
                    cout << "SemTakeTask failed to reach the waitingForGo state!\n";
                    retVal = 8;
                    break;
                }

                // Let her rip!
                startingGun.pullTrigger();

                // SemTaskTask should get to the going state.
                for (n = 0; n != 10; ++n)
                {
                    if (takeTask.getState() == SemTakeTask::State::going) break;
                    this_thread::sleep_for(chrono::milliseconds(10));
                }
                if (n == 10)
                {
                    cout << "SemTakeTask failed to reach the going state!\n";
                    takeTask.outputResults(0);
                    retVal = 9;
                    break;
                }

                // Issue a sem abort.
                sem.abort();

                // SemTaskTask should get to the aborted state.
                for (n = 0; n != 10; ++n)
                {
                    if (takeTask.getState() == SemTakeTask::State::aborted) break;
                    this_thread::sleep_for(chrono::milliseconds(10));
                }
                if (n == 10)
                {
                    cout << "SemTakeTask failed to reach the aborted state!\n";
                    retVal = 10;
                    break;
                }

            } while (false);

            // Join the thread and clear the go flag.
            sem.abort();            // Just in case we short circuited abort above.
            startingGun.abort();
            takeThread->join();

            // If we have detected a failure, report state information from the take task.
            if (0 != retVal)
            {
                takeTask.outputResults(0);
                break;
            }

        }

        // Simple pending test with one give thread running in parallel with a simple take thread.
        {
            Semaphore sem{4, 4 };  // Initially full, max 4.

            // Instantiate one give task functor.
            SemGiveTask giveTask;

            // Instantiate one thread object for a give thread.
            unique_ptr< thread > giveThread;

            // Instantiate the starting gun
            StartingGun startingGun;

            // Spawn give thread invoking give task functor. It should complete after 1 give.
            giveThread.reset( new thread{ ref(giveTask ), &startingGun, &sem, 1 } );

            // We have launched a thread. We should not short circuit our joining of it.
            // This do once structure helps ensure that we do not fail to join the thread should we bail out.
            do
            {
                // Wait for it to get to the waitingForGo state.  This should be relatively quick.
                unsigned int n = 0;
                for ( ; n != 10; ++n )
                {
                    if (giveTask.getState() == SemGiveTask::State::waitingForGo) break;
                    this_thread::sleep_for(chrono::milliseconds(10));
                }
                if ( n == 10 )
                {
                    cout << "SemGiveTask failed to reach the waitingForGo state!\n";
                    retVal = 11;
                    break;
                }

                // Let her rip!
                startingGun.pullTrigger();

                // SemGiveTask should get to the going state.
                for (n = 0; n != 10; ++n)
                {
                    if (giveTask.getState() == SemGiveTask::State::going) break;
                    this_thread::sleep_for(chrono::milliseconds(10));
                }
                if (n == 10)
                {
                    cout << "SemGiveTask failed to reach the going state!\n";
                    giveTask.outputResults(0);
                    retVal = 12;
                    break;
                }

                // At this point, SemGiveTask should be blocking on a give since it started out with a
                // maximum available count.  Verify that the available count returns 4.
                if (sem.getAvailableCount() != 4)
                {
                    cout << "Semaphore should have reported an available count of 4 with a blocked give and reported "
                    << sem.getAvailableCount() << "!" << endl;
                    retVal = 13;
                    break;
                }

                // We want to fire up a simple thread that will simply take the semaphore.
                // Instantiate one thread object for a give thread.
                auto takeFunk = [&sem]() { sem.take(); };
                unique_ptr< thread > simpleTakeThread;
                simpleTakeThread.reset( new thread{ std::ref( takeFunk ) } );
                this_thread::sleep_for( chrono::milliseconds( 100 ) );
                simpleTakeThread->join();

                // SemGiveTask should be in the completed state.
                for (n = 0; n != 10; ++n)
                {
                    if (giveTask.getState() == SemGiveTask::State::completed) break;
                    this_thread::sleep_for(chrono::milliseconds(10));
                }
                if (n == 10)
                {
                    cout << "SemGiveTask failed to reach the completed state!\n";
                    giveTask.outputResults(0);
                    retVal = 14;
                    break;
                }

                // Verify that the available count returns still returns a 4.
                if (sem.getAvailableCount() != 4)
                {
                    cout << "Semaphore should have reported an available count of 4 with a blocked give and reported "
                         << sem.getAvailableCount() << "!" << endl;
                    retVal = 15;
                    break;
                }


            } while (false);

            // Join the thread and clear the go flag.
            ///@note The abort does unblock a give. This was tested in the interim even though it
            ///is not specifically no longer tested. Testing it now requires new test specifically addressing it.
            sem.abort();            // Just in case we short circuited abort above.
            startingGun.abort();
            giveThread->join();

            // If we have detected a failure, report state information from the take task.
            if (0 != retVal)
            {
                giveTask.outputResults(0);
                break;
            }
        }

        // Multi-threaded contention testing
        {
            // Fake this as 8. VisualStudio compiler insist on a constexpr for array dimension.
            // So I can't initialize it easily at runtime with hardware_concurrency call.
            // There is a fix, but it involves a LUT and I don't feel like implementing it right now.
            // All this does is control the number of threads and I would like to ensure some overlap.
            constexpr unsigned int numCPUs = 8;

            // Our semaphore giveAndTakeCount
            constexpr unsigned int giveAndTakeCount = 524288 >> 1;

            // Here we will specify a maxAvailableCount for force SemGiveTasks to wait when it is reached.
            Semaphore sem{ 0, 1024 };  // Initially empty, max 1024

            // Put and Get Task Functors, which can return state information when the runs are complete.
            SemTakeTask takeTasks[numCPUs];
            SemGiveTask giveTasks[numCPUs];

            // Give and Take Threads
            unique_ptr< thread > takeThreads[numCPUs];
            unique_ptr< thread > giveThreads[numCPUs];

            // Instantiate the starting gun
            StartingGun startingGun;

            // Spawn the take task threads
            for (unsigned int i = 0; i != numCPUs; ++i)
            {
                takeThreads[i].reset(new thread{ref(takeTasks[i]), &startingGun, &sem, giveAndTakeCount });
            }

            // Spawn the give task threads
            for (unsigned int i = 0; i != numCPUs; ++i)
            {
                giveThreads[i].reset(new thread{ref(giveTasks[i]), &startingGun, &sem, giveAndTakeCount });
            }

            // We have launched a bunch of threads. We should not short circuit our joining of them.
            // This do once structure helps ensure that we do not fail to join each thread should we bail out.
            do {
                // Wait for the take tasks to achieve the waitingForGo state.  This should be be immediate.
                unsigned int n = 0;
                for (; n != 10; ++n)
                {
                    unsigned int i = 0;
                    for (; i != numCPUs; ++i)
                    {
                        if (takeTasks[i].getState() != SemTakeTask::State::waitingForGo) {
                            this_thread::sleep_for(chrono::milliseconds(10));
                            break;
                        }
                    }
                    if (numCPUs == i)
                    {
                        break;
                    }
                }
                if (n == 10)
                {
                    cout << "SemTakeTask failed to reach the waitingForGo state!\n";
                    retVal = 16;
                    break;
                }

                // Let her rip
                startingGun.pullTrigger();

                // Wait for the give tasks to achieve the waitingForGo state.  This should be be immediate.
                for (n = 0; n != 10; ++n)
                {
                    unsigned int i = 0;
                    for (; i != numCPUs; ++i)
                    {
                        if (giveTasks[i].getState() != SemGiveTask::State::going) {
                            this_thread::sleep_for(chrono::milliseconds(10));
                            break;
                        }
                    }
                    if (numCPUs == i)
                    {
                        break;
                    }
                }
                if (n == 10)
                {
                    cout << "SemGiveTask failed to reach the going state!\n";
                    retVal = 17;
                    break;
                }

                // Threads should be giving and taking the semaphore.
                // Poll takers for completion.
                constexpr int maxLoops = 100;
                bool failed = false;
                for (n = 0; n != maxLoops; ++n) {
                    this_thread::sleep_for(chrono::milliseconds(1000));
                    unsigned int numCompleted = 0;
                    for (unsigned int j = 0; j != numCPUs; ++j) {
                        SemTakeTask::State tState = takeTasks[j].getState();
                        if (SemTakeTask::State::going != tState && SemTakeTask::State::completed != tState) {
                            failed = true;
                            break;
                        }
                        if (tState == SemTakeTask::State::completed ) ++numCompleted;
                    }
                    // If we failed or numCompleted is the number of get threads we've spun, then all is done
                    if (failed || numCompleted == numCPUs)
                        break;
                }

                // If failed set return value and break out.
                if ( failed ) {
                    cout << "FAILED" << endl;
                    retVal = 18;
                    break;
                }

                // Obviously, the GiveTasks should all be completed too. This would be a sanity check on the test itself.

            } while(false);

            // Abort the semaphore for good measure and join all threads.
            // The abort is inconsequential if all threads (take and give) completed.
            sem.abort();
            startingGun.reload();
            for (unsigned int i = 0; i != numCPUs; ++i)
            {
                takeThreads[i]->join();
                giveThreads[i]->join();
            }

            // If no failure detected so far, the semaphore available count should be zero.
            if (0 == retVal && sem.getAvailableCount() != 0)
            {
                // If we make it here, it failed.
                cout << "Semaphore should have reported an available giveAndTakeCount of zero after threads do initial takes!\n";
                retVal = 19;
            }

            // If failure detected, then output the task status of each.
            if (0 != retVal)
            {
                for (unsigned int i = 0; i != numCPUs; ++i)
                {
                    takeTasks[i].outputResults(i);
                    giveTasks[i].outputResults(i);
                }
            }
        }

    } while ( false );

    return retVal;
}
#pragma clang diagnostic pop
