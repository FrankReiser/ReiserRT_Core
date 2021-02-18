//
// Created by frank on 2/18/21.
//


#include "Semaphore.hpp"

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <thread>

using namespace std;
using namespace ReiserRT::Core;

mutex goConditionMutexS;
condition_variable goConditionS;
atomic<bool> goFlagS{ false };

struct SemTakeTask
{
    enum class State { constructed, waitingForGo, going, unknownExceptionDetected, aborted, completed };

    SemTakeTask() = default;
    ~SemTakeTask() = default;

    void operator()(Semaphore* theSem, unsigned int nTakes)
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

private:
    const char* stateStr() const
    {
        switch (state)
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
public:

    void outputResults(unsigned int i)
    {
        cout << "SemTakeTask(" << i << ") takeCount=" << takeCount
             << ", state=" << stateStr()
             << "\n";
    }

    State state = State::constructed;
    unsigned int takeCount = 0;
};

struct SemGiveTask
{
    enum class State { constructed, waitingForGo, going, unknownExceptionDetected, aborted, overflowDetected, completed };

    SemGiveTask() = default;
    ~SemGiveTask() = default;

    void operator()(Semaphore* theSem, unsigned int nGives)
    {
        // Wait on go condition.
        state = State::waitingForGo;
        {
            unique_lock<mutex> lock(goConditionMutexS);
            goConditionS.wait(lock, [] { return goFlagS.load(); });
        }
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

private:
    const char* stateStr() const
    {
        switch (state)
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
public:

    void outputResults(unsigned int i)
    {
        cout << "SemGiveTask(" << i << ") giveCount=" << giveCount
             << ", state=" << stateStr()
             << "\n";
    }

    State state = State::constructed;
    unsigned int giveCount = 0;
};


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
            Semaphore sem{0};  // Initially empty

            // Instantiate one take task for the semaphore.
            SemTakeTask takeTask;
            unique_ptr< thread > takeThread;
            takeThread.reset(new thread{ ref(takeTask), &sem, 1 });

            // Wait for it to get to the waitingForGo state.  This should be relatively quick.
            int n = 0;
            for (; n != 10; ++n)
            {
                this_thread::sleep_for(chrono::milliseconds(10));
                if (takeTask.state == SemTakeTask::State::waitingForGo) break;
            }
            if (n == 10)
            {
                cout << "SemTakeTask failed to reach the waitingForGo state!\n";
                retVal = 4;
            }

            // Let her rip, she should block (unless it failed above).
            // We have to tell it to go regardless, or it will hang and cause an serious runtime library exception.
            {
                lock_guard<mutex> lock(goConditionMutexS);
                goFlagS.store(true);
                goConditionS.notify_all();
            }
            // If we haven't failed.
            if (0 != retVal)
            {
                // SemTaskTask should get to the going state.
                for (n = 0; n != 10; ++n)
                {
                    this_thread::sleep_for(chrono::milliseconds(10));
                    if (takeTask.state == SemTakeTask::State::going) break;
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
                    if (takeTask.state == SemTakeTask::State::aborted) break;
                }
                if (n == 10)
                {
                    cout << "SemTakeTask failed to reach the completed state!\n";
                    retVal = 6;
                }
            }
            // Join the thread and clear the go flag.
            takeThread->join();
            goFlagS.store(false);

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
            Semaphore sem{ 0 };  // Initially empty

            // Put and Get Task Functors, which can return state information when the runs are complete.
            SemTakeTask takeTasks[numCores];
            SemGiveTask giveTasks[numCores];

            // Give and Take Threads
            unique_ptr< thread > takeThreads[numCores];
            unique_ptr< thread > giveThreads[numCores];

            // Instantiate the take tasks
            for (unsigned int i = 0; i != numCores; ++i)
            {
                takeThreads[i].reset(new thread{ ref(takeTasks[i]), &sem, count });
            }

            // Instantiate the give tasks
            for (unsigned int i = 0; i != numCores; ++i)
            {
                giveThreads[i].reset(new thread{ ref(giveTasks[i]), &sem, count });
            }

            // Let her rip
            {
                lock_guard<mutex> lock(goConditionMutexS);
                goFlagS.store(true);
                goConditionS.notify_all();
            }

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
                    if (takeTasks[j].state == SemTakeTask::State::completed)
                        ++numCompleted;
                }
                if (numCompleted == numCores)
                {
                    goFlagS.store(false);
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
            goFlagS.store(false);
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
