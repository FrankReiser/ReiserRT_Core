//
// Created by frank on 2/19/21.
//

#include "ObjectQueue.hpp"

#include <random>
#include <iostream>

using namespace std;
using namespace ReiserRT::Core;

uniform_int_distribution< unsigned int > uniformDistributionOQ;
default_random_engine randEngineOQ;

constexpr unsigned int hasherOQ(unsigned int x)
{
    return (((x << 16) | (x >> 16)) ^ 0xAAAAAAAA);
}

// Movable Test class
class TestMovableOQ
{
public:
    TestMovableOQ()
            : randNum{ uniformDistributionOQ(randEngineOQ) }, randNumHash{ hasherOQ(randNum) }
    {
        ++validObjectCount;
    }

    ~TestMovableOQ()
    {
        // If not a moved from object!
        if (isValid())
        {
            --validObjectCount;
        }
    }

    TestMovableOQ(const TestMovableOQ& another) = delete;
    TestMovableOQ& operator=(const TestMovableOQ& another) = delete;

    TestMovableOQ(TestMovableOQ&& another) noexcept
    {
        randNum = another.randNum;
        randNumHash = another.randNumHash;
        another.randNum = 0;
        another.randNumHash = 0;
    }
    TestMovableOQ& operator=(TestMovableOQ&& another) noexcept
    {
        randNum = another.randNum;
        randNumHash = another.randNumHash;
        another.randNum = 0;
        another.randNumHash = 0;
        return *this;
    }

    inline bool isValid() const { return randNumHash == (unsigned int)hasherOQ(randNum); }

    static int validObjectCount;

    unsigned int randNum;
    unsigned int randNumHash;
};
int TestMovableOQ::validObjectCount = 0;



int main()
{
    int retVal = 0;

    do {
        // Put one object into the queue and then get it back out again.
        {
            const int queueSize = 4;
            using ObjectQueueType = ObjectQueue<TestMovableOQ>;
            ObjectQueueType queue(queueSize);

            // Verify running statistics at start.
            ObjectQueueType::RunningStateStats runningStateStats = queue.getRunningStateStatistics();
            if (0 != runningStateStats.runningCount)
            {
                cout << "The Object Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 0 << endl;
                retVal = 1;
                break;
            }
            if (0 != runningStateStats.highWatermark)
            {
                cout << "The Object Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 0 << endl;
                retVal = 2;
                break;
            }

            // Instantiate test object
            TestMovableOQ m{};

            // Verify TestMovableOQ::validObjectCount is 1.
            // This is a test of the test harness itself.
            if (TestMovableOQ::validObjectCount != 1)
            {
                cout << "ObjectQueue failed to verify an initial validObjectCount of one! Detected "
                    << TestMovableOQ::validObjectCount << endl;
                retVal = 3;
                break;
            }

            queue.put(m);

            // Verify TestMovableOQ::validObjectCount is still 1. We have a valid object, it is just in the queue.
            // This is a test of the test harness itself.
            if (TestMovableOQ::validObjectCount != 1)
            {
                cout << "ObjectQueue failed to verify an initial validObjectCount of one! Detected "
                    << TestMovableOQ::validObjectCount << endl;
                retVal = 4;
                break;
            }

            // Verify running state stats.
            runningStateStats = queue.getRunningStateStatistics();
            if (1 != runningStateStats.runningCount)
            {
                cout << "The Object Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 1 << endl;
                retVal = 5;
                break;
            }
            if (1 != runningStateStats.highWatermark)
            {
                cout << "The Object Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 1 << endl;
                retVal = 6;
                break;
            }

            // Verify m is no longer valid after it's been moved into the queue and away from itself.
            // This is a test of the test harness itself.
            if (m.isValid())
            {
                cout << "ObjectQueue failed test of test harness movable object!" << endl;
                retVal = 7;
                break;
            }

            // Now get the data back out.
            m = queue.get();
            // Verify m is again valid after it's been moved back out of the queue.
            if (!m.isValid())
            {
                cout << "ObjectQueue failed simple one put, one get test - returned invalid data!" << endl;
                retVal = 8;
                break;
            }

            // Verify running state stats.
            runningStateStats = queue.getRunningStateStatistics();
            if (0 != runningStateStats.runningCount)
            {
                cout << "The Object Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 0 << endl;
                retVal = 9;
                break;
            }
            if (1 != runningStateStats.highWatermark)
            {
                cout << "The Object Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 1 << endl;
                retVal = 10;
                break;
            }
        }

        // Test Put on Reserved
        {
            const int queueSize = 4;
            using ObjectQueueType = ObjectQueue<TestMovableOQ>;
            ObjectQueueType queue(queueSize);

            TestMovableOQ m{};
            auto reservedPutHandle = queue.reservePutHandle();
            queue.putOnReserved( reservedPutHandle, m );

            // Verify running state stats.
            ObjectQueueType::RunningStateStats runningStateStats = queue.getRunningStateStatistics();
            runningStateStats = queue.getRunningStateStatistics();
            if (1 != runningStateStats.runningCount)
            {
                cout << "The Object Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 1 << endl;
                retVal = 11;
                break;
            }
            if (1 != runningStateStats.highWatermark)
            {
                cout << "The Object Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 1 << endl;
                retVal = 12;
                break;
            }

        }

        ///@todo I have untested functionality.

    } while ( false );

    return retVal;
}
