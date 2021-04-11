//
// Created by frank on 2/19/21.
//

#ifndef REISERRT_RINGBUFFERGUARDEDTESTTASKS_H
#define REISERRT_RINGBUFFERGUARDEDTESTTASKS_H

#include "RingBufferGuarded.hpp"

#include <atomic>

class StartingGun;

// Class for producing verifiable data going through a RingBuffer
class ThreadTestDataRBG
{
private:
    static inline unsigned int munger(unsigned int x)
    {
        return (((x << 16) | (x >> 16)) ^ 0xAAAAAAAA);
    }

public:
    ThreadTestDataRBG();

    inline bool isValid() const { ++validatedInvocations; return randNumHash == (unsigned int)munger(randNum); }
    inline unsigned int getRandNum() const { return randNum; }
    inline unsigned int getValidatedInvocations() const {  return validatedInvocations; }

private:
    const unsigned int randNum;
    const unsigned int randNumHash;
    mutable unsigned int validatedInvocations{ 0 };
    const unsigned int pad{ 0 };
};

class PutTaskRBG {
public:
    enum class State : char {
        constructed, waitingForGo, going, unknownExceptionDetected, aborted, completed
    };
    using StateType = std::atomic<State>;

    PutTaskRBG() : state{State::constructed} {}
    ~PutTaskRBG() {}

    void operator()(StartingGun* startingGun, ReiserRT::Core::RingBufferGuarded< const ThreadTestDataRBG* >* theRing,
            const ThreadTestDataRBG* testData, unsigned int nElements);

    inline State getState() { return state.load(); }

    void outputResults(unsigned int i) const;

    const char* stateStr() const
    {
        switch (state.load())
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

private:
    unsigned int completionCount{0};
    StateType state;
};

struct GetTaskRBG {
public:
    enum class State : char {
        constructed, waitingForGo, going, invalidDataDetected, nullDataDetected,
        unknownExceptionDetected, aborted, completed
    };
    using StateType = std::atomic<State>;

    GetTaskRBG() : state{ State::constructed } {}
    ~GetTaskRBG() {}

    void operator()(StartingGun* startingGun, ReiserRT::Core::RingBufferGuarded< const ThreadTestDataRBG* >* theRing,
            unsigned int nElements);

    inline State getState() { return state.load(); }

    void outputResults(unsigned int i) const;

    const char* stateStr() const
    {
        switch (state.load())
        {
            case State::constructed: return "constructed";
            case State::waitingForGo: return "waitingForGo";
            case State::going: return "going";
            case State::invalidDataDetected: return "invalidDataDetected";
            case State::nullDataDetected: return "nullDataDetected";
            case State::unknownExceptionDetected: return "unknownExceptionDetected";
            case State::aborted: return "aborted";
            case State::completed: return "completed";
            default: return "unknown";
        }
    }

private:
    unsigned int completionCount{0};
    StateType state;

};

#endif //REISERRT_RINGBUFFERGUARDEDTESTTASKS_H
