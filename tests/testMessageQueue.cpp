//
// Created by frank on 2/19/21.
//

#include "MessageQueue.hpp"

#include <iostream>
#include <random>

using namespace std;
using namespace ReiserRT::Core;

class SimpleTestMessage : public MessageBase
{
public:
    SimpleTestMessage() noexcept = default;

    virtual ~SimpleTestMessage() noexcept = default;

    SimpleTestMessage(const SimpleTestMessage& another) noexcept = delete;

    SimpleTestMessage& operator = (const SimpleTestMessage& another) noexcept = delete;

    SimpleTestMessage(SimpleTestMessage&& another) noexcept = default;

    SimpleTestMessage& operator = (SimpleTestMessage&& another) noexcept = delete;

protected:
    virtual void dispatch() { ++dispatchCount; }

    virtual const char* name() { return "SimpleTestMessage"; }

public:
    static size_t dispatchCount;
};

size_t SimpleTestMessage::dispatchCount = 0;

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

    using MessageQueueType = MessageQueue< sizeof(ImpleMessage) >;

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
        if (randNumHash != (unsigned int)hasherMQ(randNum))
            std::cout << "MessageQueueUserProcess::onImpleMessage detected invalid data!\n";
    }

    void sendImpleMessage()
    {
        msgQueue.emplace< ImpleMessage >(this);
    }

    size_t getDispatchCount() { return numberImpleMessagesDispatched; }

private:
    ActiveContextType messageHandlerThread{};
    size_t numberImpleMessagesDispatched{ 0 };
    MessageQueueType msgQueue{ 4 };
};


int main()
{
    int retVal = 0;

    do {
        // Try a simple message put and dispatch and then a emplace and dispatch
        {
            using MessageQueueType = MessageQueue<sizeof(SimpleTestMessage)>;
            MessageQueueType msgQueue(3);

            // Verify running statistics at start.
            MessageQueueType::RunningStateStats runningStateStats = msgQueue.getRunningStateStatistics();
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

            // Get and Dispatch the message
            msgQueue.getAndDispatch();
            if (SimpleTestMessage::dispatchCount != 1)
            {
                std::cout << "Failed to read a SimpleTestMessage::dispatchCount of 1 after put/getAndDispatch, got "
                          << SimpleTestMessage::dispatchCount << endl;
                retVal = 5;
                break;
            }
            // Verify the name of the last message dispatched
            if (string{ "SimpleTestMessage" } != msgQueue.getNameOfLastMessageDispatched())
            {
                std::cout << "Failed to detected \"SimpleTestMessage\" name for last message dispatched after dispatching SimpleTestMessage" << endl;
                retVal = 6;
                break;
            }

            // Verify running statistics
            runningStateStats = msgQueue.getRunningStateStatistics();
            if (0 != runningStateStats.runningCount)
            {
                cout << "The Message Queue running count is " << runningStateStats.runningCount
                     << ". Expected " << 0 << endl;
                retVal = 7;
                break;
            }
            if (1 != runningStateStats.highWatermark)
            {
                cout << "The Message Queue highWatermark is " << runningStateStats.highWatermark
                     << ". Expected " << 1 << endl;
                retVal = 8;
                break;
            }

            // Emplace and dispatch second message.
            msgQueue.emplace< SimpleTestMessage >();
            msgQueue.getAndDispatch();
            if (SimpleTestMessage::dispatchCount != 2)
            {
                cout << "Failed to read a SimpleTestMessage::dispatchCount of 2 after second emplace/getAndDispatch, got "
                          << SimpleTestMessage::dispatchCount << endl;
                retVal = 9;
                break;
            }
        }

        // Use an Active Class to test using Imple Pointers
        {
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
            if (pUserProcess->getDispatchCount() != count)
            {
                std::cout << "Failed to read a correct MessageQueueUserProcess::dispatchCount after sendImpleMessage*count, got "
                          << SimpleTestMessage::dispatchCount << ", expected " << count << "\n";
                retVal = 10;
            }

            // Destroy it and if it doesn't crash, we're good.
            pUserProcess.reset();

        }

        cout << "Blah" << endl;

     } while ( false );

    return retVal;
}