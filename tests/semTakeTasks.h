//
// Created by frank on 2/18/21.
//

#ifndef REISERRT_SEMTAKETASKS_H
#define REISERRT_SEMTAKETASKS_H

namespace ReiserRT
{
    namespace Core
    {
        class Semaphore;
    }
}
namespace std
{
    class condition_variable;
    class mutex;
}
class SemTakeTask2
{
public:
    enum class State { constructed, waitingForGo, going, unknownExceptionDetected, aborted, completed };

    SemTakeTask2() = default;
    ~SemTakeTask2() = default;

    void operator()(ReiserRT::Core::Semaphore& theSem, unsigned int nTakes);

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

    State state{ State::constructed };
    unsigned int takeCount{ 0 };
};



#endif //REISERRT_SEMTAKETASKS_H
