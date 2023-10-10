//
// Created by frank on 2/19/21.
//

#include "MessageQueue.hpp"

#include <iostream>
#include <random>
#include <thread>
#include <atomic>

using namespace std;
using namespace ReiserRT::Core;

class SimpleTestMessage : public MessageBase
{
public:
    SimpleTestMessage() noexcept { ++instanceCount; }

    ~SimpleTestMessage() noexcept override { --instanceCount; }

    SimpleTestMessage(const SimpleTestMessage& another) noexcept = delete;

    SimpleTestMessage& operator = (const SimpleTestMessage& another) noexcept = delete;

    SimpleTestMessage(SimpleTestMessage&& /*another*/) noexcept { ++instanceCount; }

    SimpleTestMessage& operator = (SimpleTestMessage&& another) noexcept
    {
        if ( this != &another )
        {
            ++instanceCount;
        }
        return *this;
    }

protected:
    void dispatch() override { ++dispatchCount; }

    const char * name() const override { return "SimpleTestMessage"; }

public:
    static size_t dispatchCount;
    static size_t instanceCount;
};

size_t SimpleTestMessage::dispatchCount = 0;
size_t SimpleTestMessage::instanceCount = 0;


class MessageQueueUserProcess
{
private:
    using ActiveContextType = std::thread;

    class ImpleMessage : public MessageBase
    {
    public:
        ImpleMessage() = delete;

        explicit ImpleMessage(MessageQueueUserProcess* pImple)
                : MessageBase{}
                , pImple{pImple }
                , randNum{ pImple->getRandNumber() }
                , randNumHash{ MessageQueueUserProcess::getRandNumberHash( randNum ) }
        {
        }

        ImpleMessage(const ImpleMessage& another) = delete;
        ImpleMessage& operator = (const ImpleMessage& another) = delete;

        ImpleMessage(ImpleMessage&& another) = default;
        ImpleMessage& operator = (ImpleMessage&& another) = default;

    protected:
        void dispatch() override { pImple->onImpleMessage(randNum, randNumHash); }

        const char * name() const override { return "ImpleMessage"; }

    private:
        MessageQueueUserProcess* pImple;
        unsigned int randNum;
        unsigned int randNumHash;
    };

public:
    MessageQueueUserProcess() = default;

    ~MessageQueueUserProcess()
    {
        destructing = true;
        msgQueue.abort();

        if (messageHandlerThread.joinable())
            messageHandlerThread.join();
    }

    void activate()
    {
        messageHandlerThread = ActiveContextType{ [this] { messageHandlerProc(); } };
    }

    void messageHandlerProc()
    {
        try
        {
            for (;; )
            {
                msgQueue.getAndDispatch();
            }
        }
        catch (const std::exception& e)
        {
            if ( !destructing )
                cout << "Caught unexpected exception: " << e.what() << endl;
        }
    }

    void onImpleMessage(unsigned int randNum, unsigned int randNumHash)
    {
        ++numberImpleMessagesDispatched;
        if (randNumHash == hasherMQ(randNum))
            ++numberOfMessagesValidated;
    }

    void sendImpleMessage()
    {
        msgQueue.emplace< ImpleMessage >(this);
    }

    unsigned int getRandNumber() { return uniformDistributionMQ( randEngineMQ ); }
    static unsigned int getRandNumberHash( unsigned int randNum ) { return hasherMQ( randNum); }
    static unsigned int hasherMQ(unsigned int x) { return (((x << 16) | (x >> 16)) ^ 0xAAAAAAAA); }

    size_t getDispatchCount() const { return numberImpleMessagesDispatched; }
    size_t getValidatedCount() const { return numberOfMessagesValidated; }

    MessageQueueBase::AutoDispatchLock getAutoDispatchLock() { return msgQueue.getAutoDispatchLock(); }

private:
    uniform_int_distribution< unsigned int > uniformDistributionMQ;
    default_random_engine randEngineMQ{ std::random_device{}() };
    ActiveContextType messageHandlerThread{};
    size_t numberImpleMessagesDispatched{ 0 };
    size_t numberOfMessagesValidated{ 0 };

    MessageQueue msgQueue{ 256, sizeof( ImpleMessage ), true };

    atomic< bool > destructing{ false };
};

int activeUserProcessTestPrimary()
{
    int retVal=0;

    // Create MessageQueueUserProcess on heap and activate it.
    std::unique_ptr< MessageQueueUserProcess > pUserProcess{ new MessageQueueUserProcess{} };
    pUserProcess->activate();

    // Invoke its send function multiple times
    constexpr size_t count = 1048576;
    for (size_t i = 0; i != count; i++)
        pUserProcess->sendImpleMessage();

    // Wait a bit.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify it dispatched correct number of messages.
    if ( pUserProcess->getDispatchCount() != count )
    {
        std::cout << "Failed to read a correct MessageQueueUserProcess::dispatchCount after sendImpleMessage*count, got "
                  << pUserProcess->getDispatchCount() << ", expected " << count << "\n";
        retVal = 30;
    }
    // Else  Verify they were all validated
    else if ( pUserProcess->getValidatedCount() != count )
    {
        std::cout << "Failed to read a correct MessageQueueUserProcess::dispatchCount after sendImpleMessage*count, got "
                  << pUserProcess->getDispatchCount() << ", expected " << count << "\n";
        retVal = 31;
    }
//    else
//        std::cout << "PASSED activeUserProcessTestPrimary" << std::endl;

    return retVal;
}

int activeUserProcessTestSecondary()
{
    int retVal=0;

    // Create MessageQueueUserProcess on heap and activate it.
    std::unique_ptr< MessageQueueUserProcess > pUserProcess{ new MessageQueueUserProcess{} };
    pUserProcess->activate();

    do {
        // Send a simple imple message and verify a dispatch count of 1
        pUserProcess->sendImpleMessage();
        this_thread::sleep_for( chrono::milliseconds(100) );
        if ( pUserProcess->getDispatchCount() != 1 )
        {
            std::cout << "Failed to read a correct MessageQueueUserProcess::dispatchCount after sendImple Message, got "
                      << pUserProcess->getDispatchCount() << ", expected " << 1 << "\n";
            retVal = 32;
            break;
        }

        // Using scoping to hold an auto dispatch lock
        {
            // Obtain an auto dispatch lock and send a message.
            auto dispatchLock = pUserProcess->getAutoDispatchLock();
            pUserProcess->sendImpleMessage();

            // Sleep a little bit and verify that the dispatch count is still 1 from before.
            // We should be blocking dispatches.
            this_thread::sleep_for( chrono::milliseconds(100) );
            if (pUserProcess->getDispatchCount() != 1)
            {
                std::cout << "Failed to read a correct MessageQueueUserProcess::dispatchCount blocking dispatching, got "
                          << pUserProcess->getDispatchCount() << ", expected " << 1 << "\n";
                retVal = 33;
                break;
            }
        }

        // Sleep a little bit and verify that dispatch count is now 2.
        this_thread::sleep_for( chrono::milliseconds(100) );
        if (pUserProcess->getDispatchCount() != 2)
        {
            std::cout << "Failed to read a correct MessageQueueUserProcess::dispatchCount blocking dispatching, got "
                      << pUserProcess->getDispatchCount() << ", expected " << 2 << "\n";
            retVal = 34;
            break;
        }

        // Scoped Test Of AutoDispatchLock native_handle operation
        {
            // Obtain an auto dispatch lock and send a message.
            auto dispatchLock = pUserProcess->getAutoDispatchLock();
            auto dispatchLockNativeHandle = dispatchLock.native_handle();

            // The Lock should be taken. We should not be able to lock it again.
            // so we will try to lock.
            auto e = pthread_mutex_trylock( dispatchLockNativeHandle );
            if ( 0 == e ) {
                std::cout << "Failed, expected to not succeed locking already locked mutex" << std::endl;
                retVal = 35;
                break;
            }
            if ( EBUSY != e ) {
                std::cout << "Failed, expected to not succeed locking already locked mutex" << std::endl;
                retVal = 36;
                break;
            }

        }

//        std::cout << "PASSED activeUserProcessTestSecondary" << std::endl;
    } while (false);

    return retVal;
}

int main()
{
    int retVal = 0;

    do {
        // Try a simple message put and dispatch and then emplace and dispatch
        {
            MessageQueue msgQueue(3, sizeof( SimpleTestMessage ));

            // Verify running statistics at start.
            MessageQueue::RunningStateStats runningStateStats = msgQueue.getRunningStateStatistics();
            if (0 != runningStateStats.runningCount)
            {
                cout << "The Message Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 0 << endl;
                retVal = 1;
                break;
            }
            if (0 != runningStateStats.highWatermark)
            {
                cout << "The Message Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 0 << endl;
                retVal = 2;
                break;
            }

            // Put first message in message queue
            msgQueue.put( SimpleTestMessage{} );

            // Verify running statistics
            runningStateStats = msgQueue.getRunningStateStatistics();
            if (1 != runningStateStats.runningCount)
            {
                cout << "The Message Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 1 << endl;
                retVal = 3;
                break;
            }
            if (1 != runningStateStats.highWatermark)
            {
                cout << "The Message Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 1 << endl;
                retVal = 4;
                break;
            }

            // Verify SimpleTestMessage instance count;
            if ( 1 != SimpleTestMessage::instanceCount )
            {
                cout << "The SimpleTestMessage::instanceCount is " << SimpleTestMessage::instanceCount
                     << ". Expected " << 1 << endl;
                retVal = 5;
                break;
            }

            // Get and Dispatch the message
            msgQueue.getAndDispatch();
            if (SimpleTestMessage::dispatchCount != 1)
            {
                std::cout << "Failed to read a SimpleTestMessage::dispatchCount of 1 after put/getAndDispatch, got "
                          << SimpleTestMessage::dispatchCount << endl;
                retVal = 6;
                break;
            }
            // Verify the name of the last message dispatched
            if (string{ "SimpleTestMessage" } != msgQueue.getNameOfLastMessageDispatched())
            {
                std::cout << "Failed to detected \"SimpleTestMessage\" name for last message dispatched after dispatching SimpleTestMessage" << endl;
                retVal = 7;
                break;
            }

            // Verify running statistics
            runningStateStats = msgQueue.getRunningStateStatistics();
            if (0 != runningStateStats.runningCount)
            {
                cout << "The Message Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 0 << endl;
                retVal = 8;
                break;
            }
            if (1 != runningStateStats.highWatermark)
            {
                cout << "The Message Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 1 << endl;
                retVal = 9;
                break;
            }

            // Verify SimpleTestMessage instance count;
            if ( 0 != SimpleTestMessage::instanceCount )
            {
                cout << "The SimpleTestMessage::instanceCount is " << SimpleTestMessage::instanceCount
                     << ". Expected " << 0 << endl;
                retVal = 10;
                break;
            }

            // Emplace and dispatch second message.
            msgQueue.emplace< SimpleTestMessage >();
            msgQueue.getAndDispatch();
            if (SimpleTestMessage::dispatchCount != 2)
            {
                cout << "Failed to read a SimpleTestMessage::dispatchCount of 2 after second emplace/getAndDispatch, got "
                          << SimpleTestMessage::dispatchCount << endl;
                retVal = 11;
                break;
            }
        }

        // Ensure That if a message throws on construction that the MessageQueue remains invariant.
        {
            class MessageThrowOnConstruct : public MessageBase
            {
            public:
                MessageThrowOnConstruct() { throw std::exception{}; }

                ~MessageThrowOnConstruct() noexcept override = default;

                MessageThrowOnConstruct(MessageThrowOnConstruct&& another) noexcept = default;

                MessageThrowOnConstruct& operator = (MessageThrowOnConstruct&& another) noexcept = default;

            protected:
                void dispatch() override {}
            };

            MessageQueue msgQueue{ 3, sizeof( MessageThrowOnConstruct ) };

            bool testFailed = true;
            try {
                msgQueue.emplace< MessageThrowOnConstruct >( );
            }
            catch ( const std::exception & )
            {
//                cout << "CAUGHT EXPECTED EXCEPTION" << endl;
                testFailed = false;
            }

            if ( testFailed )
            {
                cout << "Expected Runtime Error which did not occur" << endl;
                retVal = 12;
                break;
            }

            // Verify running count of zero
            MessageQueue::RunningStateStats runningStateStats = msgQueue.getRunningStateStatistics();
            if (0 != runningStateStats.runningCount )
            {
                cout << "The Message Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 0 << endl;
                retVal = 13;
                break;
            }

            if (1 != runningStateStats.highWatermark)
            {
                cout << "The Message Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 1 << endl;
                retVal = 14;
                break;
            }
        }

        // Ensure That if a message throws on dispatch that the MessageQueue remains invariant.
        {
            class MessageThrowOnDispatch : public MessageBase
            {
            public:
                using MessageBase::MessageBase;

            protected:
                void dispatch() override { throw std::exception{}; }
            };

            MessageQueue msgQueue{ 3, sizeof( MessageThrowOnDispatch ) };

            msgQueue.emplace< MessageThrowOnDispatch >();
            MessageQueue::RunningStateStats runningStateStats = msgQueue.getRunningStateStatistics();
            if (1 != runningStateStats.runningCount)
            {
                cout << "The Message Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 1 << endl;
                retVal = 15;
                break;
            }
            if (1 != runningStateStats.highWatermark)
            {
                cout << "The Message Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 1 << endl;
                retVal = 16;
                break;
            }

            bool testFailed = true;
            try {
                msgQueue.getAndDispatch();
            }
            catch ( const std::exception & ){
                testFailed = false;
            }

            if ( testFailed )
            {
                cout << "Expected Runtime Error which did not occur" << endl;
                retVal = 17;
                break;
            }

            // Verify running count of zero
            runningStateStats = msgQueue.getRunningStateStatistics();
            if (0 != runningStateStats.runningCount )
            {
                cout << "The Message Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 0 << endl;
                retVal = 18;
                break;
            }

            if (1 != runningStateStats.highWatermark)
            {
                cout << "The Message Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 1 << endl;
                retVal = 19;
                break;
            }
        }

        // Test the Purge Feature
        {
            // Reset SimpleTestMessageCounters from previous tests to zero.
            SimpleTestMessage::instanceCount = 0;
            SimpleTestMessage::dispatchCount = 0;

            MessageQueue msgQueue(4, sizeof( SimpleTestMessage ));

            // Before we put any messages into the queue. Ensure purge on an empty queue does not hang.
            msgQueue.purge();

            // Put two messages in message queue
            msgQueue.put( SimpleTestMessage{} );
            msgQueue.put( SimpleTestMessage{} );

            // Verify SimpleTestMessage instance count;
            if ( 2 != SimpleTestMessage::instanceCount )
            {
                cout << "The SimpleTestMessage::instanceCount is " << SimpleTestMessage::instanceCount
                     << ". Expected " << 2 << endl;
                retVal = 21;
                break;
            }

            // Verify SimpleTestMessage dispatch count
            if ( 0 != SimpleTestMessage::dispatchCount )
            {
                cout << "The SimpleTestMessage::dispatchCount is " << SimpleTestMessage::dispatchCount
                     << ". Expected " << 0 << endl;
                retVal = 22;
                break;
            }

            // Purge the queue and verify instance count
            msgQueue.purge();
            if ( 0 != SimpleTestMessage::instanceCount )
            {
                cout << "The SimpleTestMessage::instanceCount is " << SimpleTestMessage::instanceCount
                     << ". Expected " << 0 << endl;
                retVal = 23;
                break;
            }

            // Verify SimpleTestMessage dispatch count is still 0
            if ( 0 != SimpleTestMessage::dispatchCount )
            {
                cout << "The SimpleTestMessage::dispatchCount is " << SimpleTestMessage::dispatchCount
                     << ". Expected " << 0 << endl;
                retVal = 24;
                break;
            }
        }



        // Use an Active Class to test using Imple Pointers with Messages.
        activeUserProcessTestPrimary();

        // Use an Active Class to test Dispatch Locking Feature.
        activeUserProcessTestSecondary();

     } while ( false );

    return retVal;
}
