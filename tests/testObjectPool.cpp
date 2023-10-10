//
// Created by frank on 2/19/21.
//

#include "ObjectPool.hpp"
#include "ReiserRT_CoreExceptions.hpp"

#include <iostream>
#include <forward_list>

using namespace std;
using namespace ReiserRT::Core;

class TestClassForOP1
{
public:
    TestClassForOP1() { ++objectCount; }
    virtual ~TestClassForOP1() { --objectCount; }

    static size_t objectCount;
};

size_t TestClassForOP1::objectCount = 0;


int main()
{
    int retVal = 0;

    do
    {
        // Basic functionality.
        {
            // Create an ObjectPool for our test class with room to allocate 4 objects
            using ObjectPoolType = ObjectPool<TestClassForOP1>;
            ObjectPoolType objectPool(4);

            // Verify its size and that no objects were actually created yet.
            if (objectPool.getSize() != 4) {
                cout << "Object pool should have a size of 4 and has a size of "
                     << objectPool.getSize() << endl;
                retVal = 1;
                break;
            }
            if (TestClassForOP1::objectCount != 0) {
                cout << "TestClassForOP::objectCount should have value of zero after creation of pool and is "
                     << TestClassForOP1::objectCount << endl;
                retVal = 2;
                break;
            }

            // ObjectPools start out with the low watermark and running count set to the pool size.
            // That is because these progress downward as you start creating objects.
            // Verify that this is true.
            ObjectPoolType::RunningStateStats runningStateStats = objectPool.getRunningStateStatistics();
            if (objectPool.getSize() != runningStateStats.lowWatermark) {
                cout << "The Object Pool low watermark is " << runningStateStats.lowWatermark
                     << ". Expected " << objectPool.getSize() << endl;
                retVal = 3;
                break;
            }
            if (objectPool.getSize() != runningStateStats.runningCount) {
                cout << "The Object Pool running count is " << runningStateStats.runningCount
                     << ". Expected " << objectPool.getSize() << endl;
                retVal = 4;
                break;
            }

            // Create an object, verify count of 1 and then allow it to go out scope.
            {
                ObjectPool<TestClassForOP1>::ObjectPtrType op = objectPool.createObj<TestClassForOP1>();
                if (TestClassForOP1::objectCount != 1) {
                    cout << "TestClassForOP::objectCount should have value of 1 after creation of first object and is "
                         << TestClassForOP1::objectCount << endl;
                    retVal = 5;
                    break;
                }

                // Verify running statistics. The low watermark should be the both reduced by one.
                runningStateStats = objectPool.getRunningStateStatistics();
                if (objectPool.getSize() - 1 != runningStateStats.lowWatermark) {
                    cout << "The Object Pool low watermark is " << runningStateStats.lowWatermark
                         << ". Expected " << objectPool.getSize() - 1 << endl;
                    retVal = 6;
                    break;
                }
                if (objectPool.getSize() - 1 != runningStateStats.runningCount) {
                    cout << "The Object Pool running count is " << runningStateStats.runningCount
                         << ". Expected " << objectPool.getSize() - 1 << endl;
                    retVal = 7;
                    break;
                }
            }

            // The object created above has gone out of scope. Verify an object count of zero and running statistics.
            if (TestClassForOP1::objectCount != 0) {
                cout << "TestClassForOP::objectCount should have value of zero after objects go out of scope"
                     << TestClassForOP1::objectCount << endl;
                retVal = 8;
                break;
            }
            runningStateStats = objectPool.getRunningStateStatistics();
            if (objectPool.getSize() - 1 != runningStateStats.lowWatermark) {
                cout << "The Object Pool low watermark is " << runningStateStats.lowWatermark
                     << ". Expected " << objectPool.getSize() - 1 << endl;
                retVal = 9;
                break;
            }
            if (objectPool.getSize() != runningStateStats.runningCount) {
                cout << "The Object Pool running count is " << runningStateStats.runningCount
                     << ". Expected " << objectPool.getSize() << endl;
                retVal = 10;
                break;
            }

            // Create 4 objects, hanging onto them, verify count, attempt to create one too many, verify throw and then,
            // allow all to go out of scope.
            {
                forward_list<ObjectPool<TestClassForOP1>::ObjectPtrType> opList;
                for (unsigned int i = 0; i != objectPool.getSize(); ++i)
                    opList.push_front(objectPool.createObj<TestClassForOP1>());

                // Verify we created 4 objects.
                if (objectPool.getSize() != TestClassForOP1::objectCount) {
                    cout << "TestClassForOP::objectCount should have value of 4 after creation of 4 objects and is "
                         << TestClassForOP1::objectCount << endl;
                    retVal = 11;
                    break;
                }

                // Verify it throws if we allocate one too many
                try {
                    opList.push_front(objectPool.createObj<TestClassForOP1>());
                    // If here, it didn't throw.  This is a failure.
                    cout << "Object pool should have thrown an error on 5th createObj attempt and did not" << endl;
                    retVal = 12;
                    break;
                }
                catch (const RingBufferUnderflow &) {
//                    std::cout << "Caught expected underflow_error exception, what = " << e.what() << endl;
                }

                // Verify running statistics. The low watermark should and the running count should be zero
                runningStateStats = objectPool.getRunningStateStatistics();
                if (0 != runningStateStats.lowWatermark) {
                    cout << "The Object Pool low watermark is " << runningStateStats.lowWatermark
                         << ". Expected " << 0 << endl;
                    retVal = 13;
                    break;
                }
                if (0 != runningStateStats.runningCount) {
                    cout << "The Object Pool running count is " << runningStateStats.runningCount
                         << ". Expected " << 0 << endl;
                    retVal = 14;
                    break;
                }
            }

            // The objects created above have gone out of scope. Verify an object count of zero and running statistics.
            if (TestClassForOP1::objectCount != 0) {
                cout << "TestClassForOP::objectCount should have value of zero after objects go out of scope"
                     << TestClassForOP1::objectCount << endl;
                retVal = 15;
                break;
            }
            runningStateStats = objectPool.getRunningStateStatistics();
            if (0 != runningStateStats.lowWatermark) {
                cout << "The Object Pool low watermark is " << runningStateStats.lowWatermark
                     << ". Expected " << 0 << endl;
                retVal = 16;
                break;
            }
            if (objectPool.getSize() != runningStateStats.runningCount) {
                cout << "The Object Pool running count is " << runningStateStats.runningCount
                     << ". Expected " << objectPool.getSize() << endl;
                retVal = 17;
                break;
            }
        }
        // Simple testing with a type hierarchy
        {
            // This class merely adds additional size to TestClassForOP for ObjectPool hierarchical testing.
            class TestClassForOP2 : public TestClassForOP1
            {
            public:
                TestClassForOP2() = default;
                ~TestClassForOP2() override = default;
                long dummy1{ 0 };    // Just make it bigger than TestClassForOP1
                long dummy2{ 0 };    // Just make it bigger than TestClassForOP1
                long dummy3{ 0 };    // Just make it bigger than TestClassForOP1
                long dummy4{ 0 };    // Just make it bigger than TestClassForOP1
            };

#if 0
            using ObjectPtrType = ObjectPool< TestClassForOP1, sizeof(TestClassForOP2) >::ObjectPtrType;
            ObjectPool< TestClassForOP1, sizeof(TestClassForOP2) > objectPool(4);
#else
            using ObjectPtrType = ObjectPool< TestClassForOP1 >::ObjectPtrType;
            ObjectPool< TestClassForOP1 > objectPool(4, sizeof(TestClassForOP2) );
#endif

            // Attempt to create a derived object from the pool
            ObjectPtrType op = objectPool.createObj< TestClassForOP2 >();

            if (TestClassForOP1::objectCount != 1)
            {
                cout << "TestClassForOP::objectCount should have value of 1 after creation of 1 TestClassForOP2 object and is "
                     << TestClassForOP1::objectCount << endl;
                retVal = 18;
                break;
            }
        }

        // Shared pointer testing. Here we want to ensure that a shared pointer obtains and correctly utilizes the Deleter
        // associated with the unique_ptr
        {
            using TestPoolType = ObjectPool< TestClassForOP1 >;
            using TestPtrType = TestPoolType::ObjectPtrType;
            using TestSharedPtrType = std::shared_ptr< TestClassForOP1 >;
            TestPoolType testPool(4);

            // Cycle the pool through 2x to ensure that shared pointer actually returns to the pool when it goes out of scope.
            // Object count testing should always indicate one.
            size_t loops = testPool.getSize() * 2;
            for (size_t i = 0; i != loops; ++i)
            {
                TestPtrType testPtr = testPool.createObj<TestClassForOP1>();
                const TestSharedPtrType testSharedPtr{ std::move(testPtr) };

                // See if it copies okay
                TestSharedPtrType testSharedPtr2{ testSharedPtr };

                // Reference testSharedPtr2 for count in order to force it to stay in scope long enough to ensure it isn't destroyed.
                if (testSharedPtr2->objectCount != 1)
                {
                    cout << "Our testSharedPtr2->objectCount should have value of 1 after creation of first object and is "
                         << testSharedPtr2->objectCount << endl;
                    retVal = 19;
                    break;
                }
            }

        }

        // Test ObjectPool requirement that a throw on a createObj construction leaves pool invariant.
        {
            // A new hierarchy based off of a abstract base class.
            class TestClassBaseForOP
            {
            public:
                TestClassBaseForOP() = default;
                virtual ~TestClassBaseForOP() = default;

                virtual int getClassID() = 0;
            };

            // This one is designed to throw on construction to validate ObjectPool invariant!
            class TestClassDerivedForOP1 : public TestClassBaseForOP
            {
            public:
                TestClassDerivedForOP1() { throw std::exception{}; }

                int getClassID() override { return 1; }
            };

            class TestClassDerivedForOP2 : public TestClassBaseForOP
            {
            public:
                TestClassDerivedForOP2() = default;

                int getClassID() override { return 2; }
            };

            using TestPoolType = ObjectPool< TestClassBaseForOP >;
            using TestPtrType = TestPoolType::ObjectPtrType;
            TestPoolType testPool(4, sizeof(TestClassDerivedForOP1));

            // Now create an object of TestClassDerivedForOP1 which is designed to throw and verify ObjectPool invariant.
            try
            {
                testPool.createObj< TestClassDerivedForOP1 >();

                // If we find ourselves here, then the test failed.
                cout << "Create of object TestClassDerivedForOP1 designed to throw on constructor did not throw!" << endl;
                retVal = 20;
                break;
            }
            catch (const exception & )
            {
                TestPoolType::RunningStateStats runningStateStats = testPool.getRunningStateStatistics();
                if (runningStateStats.runningCount != 4)
                {
                    cout << "Expected running count of 4 after object creation throw on construction! got "
                        << runningStateStats.runningCount << endl;
                    retVal = 21;
                    break;
                }
                if (runningStateStats.lowWatermark != 3)
                {
                    cout << "Expected low watermark of 3 after object creation throw on construction! got "
                        << runningStateStats.lowWatermark << endl;
                    retVal = 22;
                    break;
                }

                // At this point, we should be able to create 4 objects that don't throw underflow
                forward_list< TestPtrType > tpList;
                try
                {
                    for (size_t i = 0; i != 4; ++i)
                    {
                        tpList.push_front(testPool.createObj< TestClassDerivedForOP2 >());
                    }
                }
                catch ( const RingBufferUnderflow & )
                {
                    // If we find ourself here, then the test failed.
                    cout << "Create of object TestClassDerivedForOP2 x4 should not have resulted in underflow!" << endl;
                    retVal = 23;
                    break;
                }

                // Clear list, all objects should be returned to the pool.
                tpList.clear();
                if (runningStateStats.runningCount != 4)
                {
                    cout << "Expected running count of 4 after returning all object to the pool! got "
                        << runningStateStats.runningCount << endl;
                    retVal = 24;
                    break;
                }
            }
        }

    } while ( false );

    return retVal;
}
