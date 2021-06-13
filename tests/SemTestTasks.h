//
// Created by frank on 2/18/21.
//

#ifndef REISERRT_SEMTESTTASKS_H
#define REISERRT_SEMTESTTASKS_H

#include <atomic>

namespace ReiserRT
{
    namespace Core
    {
        class Semaphore;
    }
}
class StartingGun;

class SemTakeTask
{
public:
    enum class State { constructed, waitingForGo, going, unknownExceptionDetected, aborted, completed };
    using StateType = std::atomic<State>;

    SemTakeTask() = default;
    ~SemTakeTask() = default;

    void operator()(StartingGun* startingGun, ReiserRT::Core::Semaphore* theSem, unsigned int nTakes);

private:
    const char* stateStr() const;

public:

    void outputResults(unsigned int i);
    inline State getState() { return state.load(); }

private:
    StateType state{ State::constructed };
    unsigned int takeCount{ 0 };
};

struct SemGiveTask
{
    enum class State { constructed, waitingForGo, going, unknownExceptionDetected, aborted, completed };
    using StateType = std::atomic<State>;

    SemGiveTask() = default;
    ~SemGiveTask() = default;

    void operator()(StartingGun* startingGun, ReiserRT::Core::Semaphore* theSem, unsigned int nGives);

private:
    const char* stateStr() const;

public:

    void outputResults(unsigned int i);
    inline State getState() { return state.load(); }

private:
    StateType state{ State::constructed };
    unsigned int giveCount{ 0 };
};


#endif //REISERRT_SEMTESTTASKS_H
