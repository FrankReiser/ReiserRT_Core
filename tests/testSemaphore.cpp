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
    cout << "Hello world" << endl;
    int retVal = 0;

    do {
        // Construction, no-wait wait testing
        {
            Semaphore sem{ 4 };
            if (sem.getAvailableCount() != 4)
            {
                cout << "Semaphore should have reported an available count of 4 and reported " << sem.getAvailableCount() << "!" << endl;
                retVal = 1;
                break;
            }

            // Take all.  We should not block here.  Hanging would be a problem.
            for (int i = 0; i != 4; ++i)
                sem.wait();
            if (sem.getAvailableCount() != 0)
            {
                cout << "Semaphore should have reported an available count of 0 after 4 takes and reported " << sem.getAvailableCount() << "!" << endl;
                retVal = 2;
                break;
            }

            // Signal four and verify count of 4
            for (int i = 0; i != 4; ++i)
            {
                sem.notify();
            }
            if (sem.getAvailableCount() != 4)
            {
                cout << "Semaphore should have reported an available count of 4 after 4 gives and reported " << sem.getAvailableCount() << "!" << endl;
                retVal = 3;
                break;
            }
        }

        // Simple pending and abort testing with one take thread running alongside this thread.
        {
            StartingGun startingGun;
            Semaphore sem{0};  // Initially empty

            // Instantiate one take task for the semaphore.
            SemTakeTask2 takeTask;

            unique_ptr< thread > takeThread;
            takeThread.reset(new thread{ ref(takeTask), &startingGun, &sem, 1 });

            // Wait for it to get to the waitingForGo state.  This should be relatively quick.
            int n = 0;
            for (; n != 10; ++n)
            {
                this_thread::sleep_for(chrono::milliseconds(10));
                if (takeTask.state == SemTakeTask2::State::waitingForGo) break;
            }
            if (n == 10)
            {
                cout << "SemTakeTask failed to reach the waitingForGo state!\n";
                retVal = 4;
            }

            // Let her rip!
            startingGun.pullTrigger();

            // If we haven't failed.
            if (0 != retVal)
            {
                // SemTaskTask should get to the going state.
                for (n = 0; n != 10; ++n)
                {
                    this_thread::sleep_for(chrono::milliseconds(10));
                    if (takeTask.state == SemTakeTask2::State::going) break;
                }
                if (n == 10)
                {
                    cout << "SemTakeTask failed to reach the going state!\n";
                    takeTask.outputResults(0);
                    retVal = 5;
                }
            }

            // Issue a sem abort regardless of the failures from above or it may never end and cannot be joined.
            sem.abort();

            // If we haven't failed yet
            if (0 == retVal)
            {
                // SemTaskTask should get to the completed state.
                for (n = 0; n != 10; ++n)
                {
                    this_thread::sleep_for(chrono::milliseconds(10));
                    if (takeTask.state == SemTakeTask2::State::aborted) break;
                }
                if (n == 10)
                {
                    cout << "SemTakeTask failed to reach the completed state!\n";
                    retVal = 6;
                }
            }
            // Join the thread and clear the go flag.
            takeThread->join();
            startingGun.reload();

            if (0 != retVal)
            {
                takeTask.outputResults(0);
                break;
            }

        }

        // Multi-threaded contention testing
        {
            // Fake this as 8. VisualStudio compiler insist on a constexpr for array dimension.
            // So I can't initialize it at runtime with hardware_concurrency call.
            constexpr unsigned int numCores = 8;

            // Our Semaphore Initial Count
            constexpr unsigned int count = 524288;
            StartingGun startingGun;
            Semaphore sem{ 0 };  // Initially empty

            // Put and Get Task Functors, which can return state information when the runs are complete.
            SemTakeTask2 takeTasks[numCores];
            SemGiveTask2 giveTasks[numCores];

            // Give and Take Threads
            unique_ptr< thread > takeThreads[numCores];
            unique_ptr< thread > giveThreads[numCores];

            // Instantiate the take tasks
            for (unsigned int i = 0; i != numCores; ++i)
            {
                takeThreads[i].reset(new thread{ ref(takeTasks[i]), &startingGun, &sem, count });
            }

            // Instantiate the give tasks
            for (unsigned int i = 0; i != numCores; ++i)
            {
                giveThreads[i].reset(new thread{ ref(giveTasks[i]), &startingGun, &sem, count });
            }

            // Let her rip
            startingGun.pullTrigger();

            // Threads should be giving and taking the semaphore.
            // Poll takers for completion.
            unsigned int n;
            constexpr int maxLoops = 400;
            for (n = 0; n != maxLoops; ++n)
            {
                this_thread::sleep_for(chrono::milliseconds(500));
                unsigned int numCompleted = 0;
                for (unsigned int j = 0; j != numCores; ++j)
                {
                    if (takeTasks[j].state == SemTakeTask2::State::completed)
                        ++numCompleted;
                }
                if (numCompleted == numCores)
                {
                    startingGun.reload();
                    break;
                }
            }
            // If n got to N, then this is bad.
            if (n == maxLoops)
            {
                cout << "Timed out waiting for taker tasks to get to the completed state!\n";
                retVal = 7;
            }

            // Abort the semaphore for good measure and join all threads.
            // The abort is inconsequential if all threads (take and give) completed.
            sem.abort();
            startingGun.reload();
            for (unsigned int i = 0; i != numCores; ++i)
            {
                takeThreads[i]->join();
                giveThreads[i]->join();
            }

            // If no failure detected so far, the semaphore available count should be zero.
            if (0 == retVal && sem.getAvailableCount() != 0)
            {
                // If we make it here, it failed.
                cout << "Semaphore should have reported an available count of zero after threads do initial takes!\n";
                retVal = 8;
            }

            // If failure detected, then output the task status of each.
            if (0 != retVal)
            {
                for (unsigned int i = 0; i != numCores; ++i)
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
