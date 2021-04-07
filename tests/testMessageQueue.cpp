//
// Created by frank on 2/19/21.
//

#include "MessageQueue.hpp"

#include <iostream>
#include <random>
#include <thread>

using namespace std;
using namespace ReiserRT::Core;

class SimpleTestMessage : public MessageBase
{
public:
    SimpleTestMessage() noexcept { ++instanceCount; }

    virtual ~SimpleTestMessage() noexcept { --instanceCount; }

    SimpleTestMessage(const SimpleTestMessage& another) noexcept = delete;

    SimpleTestMessage& operator = (const SimpleTestMessage& another) noexcept = delete;

    SimpleTestMessage(SimpleTestMessage&& another) noexcept { ++instanceCount; }

    SimpleTestMessage& operator = (SimpleTestMessage&& another) noexcept
    {
        if ( this != &another )
        {
            ++instanceCount;
        }
        return *this;
    }

protected:
    virtual void dispatch() { ++dispatchCount; }

    virtual const char* name() const { return "SimpleTestMessage"; }

public:
    static size_t dispatchCount;
    static size_t instanceCount;
};

size_t SimpleTestMessage::dispatchCount = 0;
size_t SimpleTestMessage::instanceCount = 0;

uniform_int_distribution< unsigned int > uniformDistributionMQ;
default_random_engine randEngineMQ;
constexpr unsigned int hasherMQ(unsigned int x)
{
    return (((x << 16) | (x >> 16)) ^ 0xAAAAAAAA);
}


class MessageQueueUserProcess
{
private:
    using ActiveContextType = std::thread;

    class ImpleMessage : public MessageBase
    {
    public:
        ImpleMessage() = delete;

        ImpleMessage(MessageQueueUserProcess* pImple)
                : MessageBase{}
                , _pImple{ pImple }
                , randNum{ uniformDistributionMQ(randEngineMQ) }
                , randNumHash{ hasherMQ(randNum) }
        {
        }

        ImpleMessage(const ImpleMessage& another) = delete;
        ImpleMessage& operator = (const ImpleMessage& another) = delete;

        ImpleMessage(ImpleMessage&& another) = default;
        ImpleMessage& operator = (ImpleMessage&& another) = default;

    protected:
        virtual void dispatch() { _pImple->onImpleMessage(randNum, randNumHash); }

        virtual const char* name() { return "ImpleMessage"; }

    private:
        MessageQueueUserProcess* _pImple;
        unsigned int randNum;
        unsigned int randNumHash;
    };

public:
    MessageQueueUserProcess() {}

    ~MessageQueueUserProcess()
    {
        msgQueue.abort();

        if (messageHandlerThread.joinable())
            messageHandlerThread.join();
    }

    void activate()
    {
        messageHandlerThread = move(ActiveContextType{ [this] { messageHandlerProc(); } });
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
            ///@todo Catch other exceptions and also report what was caught?
        catch (const std::runtime_error&)
        {
        }
    }

    void onImpleMessage(unsigned int randNum, unsigned int randNumHash)
    {
        ++numberImpleMessagesDispatched;
        if (randNumHash == (unsigned int)hasherMQ(randNum))
        {
            ++numberOfMessagesValidated;
        }
    }

    void sendImpleMessage()
    {
        msgQueue.emplace< ImpleMessage >(this);
    }

    size_t getDispatchCount() { return numberImpleMessagesDispatched; }
    size_t getValidatedCount() { return numberOfMessagesValidated; }

    MessageQueueBase::AutoDispatchLock getAutoDispatchLock() { return std::move( msgQueue.getAutoDispatchLock() ); }

private:
    ActiveContextType messageHandlerThread{};
    size_t numberImpleMessagesDispatched{ 0 };
    size_t numberOfMessagesValidated{ 0 };

    MessageQueue msgQueue{ 4, sizeof( ImpleMessage ), true };
};


int main()
{
    int retVal = 0;

    do {
        // Try a simple message put and dispatch and then a emplace and dispatch
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
            msgQueue.put(std::move(SimpleTestMessage{}));

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
                MessageThrowOnConstruct()  { throw std::runtime_error{ "This Message Throws On Construction!" }; }

                virtual ~MessageThrowOnConstruct() noexcept = default;

                MessageThrowOnConstruct(MessageThrowOnConstruct&& another) noexcept = default;

                MessageThrowOnConstruct& operator = (MessageThrowOnConstruct&& another) noexcept = default;

            protected:
                virtual void dispatch() {}
            };

            MessageQueue msgQueue{ 3, sizeof( MessageThrowOnConstruct ) };

            bool testFailed = true;
            try {
                msgQueue.emplace< MessageThrowOnConstruct >( );
            }
            catch ( const std::runtime_error & )
            {
//                cout << "CAUGHT EXPECTED EXCEPTION" << endl;
                testFailed = false;
            }

            if ( testFailed )
            {
                if ( 0 == retVal )
                {
                    cout << "Expected Runtime Error which did not occur" << endl;
                    retVal = 12;
                }
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
                virtual void dispatch() { throw std::runtime_error{ "This Message Throws On Construction!"}; }
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
            catch ( const std::runtime_error & ){
                testFailed = false;
            }

            if ( testFailed )
            {
                if ( 0 == retVal )
                {
                    cout << "Expected Runtime Error which did not occur" << endl;
                    retVal = 17;
                }
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

        // Use an Active Class to test using Imple Pointers
        {
            // Create MessageQueueUserProcess on heap and activate it.
            std::unique_ptr< MessageQueueUserProcess > pUserProcess{ new MessageQueueUserProcess{} };
            pUserProcess->activate();

            // Invoke its send function multiple times
//            constexpr size_t count = 1048576;
            constexpr size_t count = 1048576 << 2;
            for (size_t i = 0; i != count; i++)
                pUserProcess->sendImpleMessage();

            // Wait a bit.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            // Verify it dispatched correct number of messages.
            if (pUserProcess->getDispatchCount() != count)
            {
                std::cout << "Failed to read a correct MessageQueueUserProcess::dispatchCount after sendImpleMessage*count, got "
                          << pUserProcess->getDispatchCount() << ", expected " << count << "\n";
                retVal = 20;
            }
            else
            {
                // Verify they were all validated
                if ( pUserProcess->getValidatedCount() != count )
                {
                    std::cout << "Failed to read a correct MessageQueueUserProcess::dispatchCount after sendImpleMessage*count, got "
                              << pUserProcess->getDispatchCount() << ", expected " << count << "\n";
                    retVal = 21;
                }
            }

            // Destroy it and if it doesn't crash, we're good.
            pUserProcess.reset();
        }

        // Use an Active Class to test dispatch locking
        {
            // Create MessageQueueUserProcess on heap and activate it.
            std::unique_ptr< MessageQueueUserProcess > pUserProcess{ new MessageQueueUserProcess{} };
            pUserProcess->activate();

            // Send a simple imple message and verify a dispatch count of 1
            pUserProcess->sendImpleMessage();
            this_thread::sleep_for( chrono::milliseconds(100) );
            if (pUserProcess->getDispatchCount() != 1)
            {
                std::cout << "Failed to read a correct MessageQueueUserProcess::dispatchCount after sendImple Message, got "
                          << pUserProcess->getDispatchCount() << ", expected " << 1 << "\n";
                retVal = 22;
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
                    retVal = 23;
                }
            }

            // Sleep a little bit and verify that dispatch count is now 2.
            this_thread::sleep_for( chrono::milliseconds(100) );
            if (pUserProcess->getDispatchCount() != 2)
            {
                std::cout << "Failed to read a correct MessageQueueUserProcess::dispatchCount blocking dispatching, got "
                          << pUserProcess->getDispatchCount() << ", expected " << 2 << "\n";
                retVal = 24;
            }

            // Destroy it and if it doesn't crash, we're good.
            pUserProcess.reset();
        }

     } while ( false );

    return retVal;
}
