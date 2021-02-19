//
// Created by frank on 2/18/21.
//

#ifndef REISERRT_SEMTESTTASKS_H
#define REISERRT_SEMTESTTASKS_H

namespace ReiserRT
{
    namespace Core
    {
        class Semaphore;
    }
}
class StartingGun;

class SemTakeTask2
{
public:
    enum class State { constructed, waitingForGo, going, unknownExceptionDetected, aborted, completed };

    SemTakeTask2() = default;
    ~SemTakeTask2() = default;

    void operator()(StartingGun* startingGun, ReiserRT::Core::Semaphore* theSem, unsigned int nTakes);

private:
    const char* stateStr() const;

public:

    void outputResults(unsigned int i);

    State state{ State::constructed };
    unsigned int takeCount{ 0 };
};

struct SemGiveTask2
{
    enum class State { constructed, waitingForGo, going, unknownExceptionDetected, aborted, overflowDetected, completed };

    SemGiveTask2() = default;
    ~SemGiveTask2() = default;

    void operator()(StartingGun* startingGun, ReiserRT::Core::Semaphore* theSem, unsigned int nGives);

private:
    const char* stateStr() const;

public:

    void outputResults(unsigned int i);

    State state{ State::constructed };
    unsigned int giveCount{ 0 };
};


#endif //REISERRT_SEMTESTTASKS_H
