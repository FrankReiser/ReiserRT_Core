/**
* @file JobTask.cpp
* @brief The Implementation file for some abstract JobTask.
* @authors Frank Reiser
* @date Created on June 21, 2021
*/


#include "JobTask.hpp"

// We are working with ObjectPool originated pointer types here even though CLang-CTidy does not comprehend.
#include "ObjectPool.hpp"

#include "MessageQueue.hpp"

#include "JobData.hpp"

#include <atomic>
#include <thread>
#include <chrono>
#include <iostream>

class JobTask::Imple
{
private:
    friend class JobTask;

    enum class State : int32_t
    {
        Defunct = -1,
        Constructed,
        Activating,
        Activated,
    };

    class BaseMessage : public ReiserRT::Core::MessageBase
    {
    public:
        BaseMessage() = delete;
        explicit BaseMessage( Imple * pTheImple ) : pImple{ pTheImple } {}

        BaseMessage( const BaseMessage & another ) = default;
        BaseMessage & operator =( const BaseMessage & another ) = default;

        BaseMessage( BaseMessage && another ) = default;
        BaseMessage & operator =( BaseMessage && another ) = default;

    protected:
        virtual ~BaseMessage() = default;

        // We do not do anything here. Override or accept the default.
        virtual void dispatch() {}

        Imple * pImple;	// Need to qualify to disambiguate between MessageQueueBase::Imple
    };

    class TicklerMessage : public BaseMessage
    {
    public:
        using BaseMessage::BaseMessage;
        using BaseMessage::operator=;
    };

    class DoJobMessage : public BaseMessage
    {
    public:
        DoJobMessage() = delete;
        DoJobMessage(Imple* pTheImple, JobDataPtrType && pTheJob )
                : BaseMessage{ pTheImple }
                , pJob{ std::move( pTheJob ) }
        {
        }

        DoJobMessage(const DoJobMessage & another ) = delete;
        DoJobMessage & operator =(const DoJobMessage & another ) = delete;

        DoJobMessage(DoJobMessage && another ) = default;
        DoJobMessage & operator =(DoJobMessage && another ) = default;

        virtual void dispatch() { pImple->onDoJobMessage(std::move(pJob)) ; }

        JobDataPtrType pJob;
    };

    explicit Imple( unsigned theTaskId )
        : messageQueue{ 4, sizeof( DoJobMessage ) }   // Use largest message size here or just pad it.
        , taskId(theTaskId)
    {
    }

    ~Imple()
    {
        // Capture State and immediately go Defunct.
        State destructState = state;
        state = State::Defunct;

        switch ( destructState )
        {
            // If we are Activated or Activating. We should post a tickler message but we only need to
            // if the queue is empty. Otherwise, it will figure it we're defunct without help. Additionally,
            // we certainly do not want to block on a full message queue.
            case State::Activating:
            case State::Activated:
            {
                const auto msgQueueStats = messageQueue.getRunningStateStatistics();
                if ( msgQueueStats.runningCount == 0 )
                    messageQueue.emplace<TicklerMessage>( this );
            }
            default:
                break;
        }

        if ( msgQueueProcThread.joinable() )
            msgQueueProcThread.join();
    }

    void activate()
    {
        // We have to successfully go from the Constructed state to the Activating state.
        // We are the only function allowed to make this transition. This must be honored
        // throughout the code.
        State currentState = state;
        do
        {   // If the current state is not Constructed, then we cannot transition and we bail out.
            if ( currentState != State::Constructed ) return;
        } while ( !state.compare_exchange_weak( currentState, State::Activating ) );

        // We have successfully transitioned to Activating.
        // Instantiate a lambda function capturing "this" and wrapping the messageQueueProc function.
        // Then move that into our thread instance. It starts running immediately.
        // Lambdas are tricky. Do not capture references to locals that go out of scope when we return!
        msgQueueProcThread = move( std::thread{ [this]() { messageQueueProc(); } } );

        // Now we attempt to go to the Activated state. The only thing that should prevent this from
        // happening is our destructor or deactivate operation being invoked in a race.
        currentState = state;
        do
        {   // If the current state is not Activating, then we cannot transition and we bail out.
            if ( currentState != State::Activating ) return;
        } while ( !state.compare_exchange_weak( currentState, State::Activated ) );
    }

    void deactivate()
    {
        // If we are not Activated or Activating, silently ignore request.
        if ( State::Activating > state )
            return;

        // Set state to Constructed and queue up a tickler message to wake our message queue handler.
        state = State::Constructed;
        messageQueue.emplace<TicklerMessage>( this );
    }

    void doJob( JobDataPtrType && pJobData )
    {
        if ( State::Activated == state )
            messageQueue.emplace<DoJobMessage>( this, std::move( pJobData ) );
    }

    void onDoJobMessage( JobDataPtrType && pJobData )
    {
#if 1
        std::this_thread::sleep_for( std::chrono::milliseconds( pJobData->estimatedEffortMSecs));
        if ( jobCompleteNotifier )
            jobCompleteNotifier( std::move( pJobData ) );
#else
        ///@todo Having trouble getting this to work. Just sleep for now.
        using ClockType = std::chrono::steady_clock;
        using TimePointType = std::chrono::steady_clock::time_point;
        using DurationType = std::chrono::milliseconds;

        const TimePointType t0 = ClockType::now();
        while ( State::Activated == state )
        {
            std::this_thread::sleep_for( DurationType{ 10 } );
            const DurationType elapsedTime{ ( ClockType::now() - t0 ).count() };
            if ( elapsedTime.count() > pJobData->estimatedEffortMSecs )
            {
                if ( jobCompleteNotifier )
                    jobCompleteNotifier( std::move( pJobData ) );
                break;
            }
        }
#endif
    }

    void registerJobCompleteNotifier( JobCompleteNotifier && notifier )
    {
        if ( State::Constructed != state )
        {
            std::cerr << "Registering JobCompleteNotifier must be invoked prior to activating!" << std::endl;
            return;
        }
        jobCompleteNotifier = std::move( notifier );
    }

    // Message Handling Thread Procedure.
    void messageQueueProc()
    {
#ifdef REISER_RT_HAS_PTHREADS
        // We are assuming a Linux here that supports these non-portable functions being available for this demo.
        std::string taskName{ "JobTaskMQH" + std::to_string(taskId)};// Job Task Message Queue Handler #N
        if ( pthread_setname_np( pthread_self(), taskName.c_str() ) )
        {
            std::cout << "Thread naming failed for " << taskName << ". Proceeding anyway." << std:: endl;
        }

        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        CPU_SET( taskId, &cpuSet );
        if ( pthread_setaffinity_np( pthread_self(), sizeof( cpu_set_t ), &cpuSet ) != 0 )
        {
            std::cout << "Thread Affinity failed for " << taskName << ". Proceeding anyway." << std:: endl;
        }
#endif
        // We runJobs will while we are greater than or equal to the Activating state
        while ( State::Activating <= state )
        {
            // Try just in case our message dispatch implementations throw an exception.
            try
            {
                messageQueue.getAndDispatch();
            }
            // We do not expect to see exceptions.
            catch ( const std::exception & e )
            {
                // Set Defunct state and join the timer proc thread if it actually got started.
                state = State::Defunct;
                std::string tempStr = "Message processing exception caught: ";
                tempStr += e.what();

                ///@todo Maybe report this up? We are no longer able to do jobs.
                std::cerr << tempStr << std::endl;
                continue;
            }
        }
    }

    ReiserRT::Core::MessageQueue messageQueue;
    std::thread msgQueueProcThread{};

    // Default construction for these is fine. The thread does not start until I move a function into it.
    JobCompleteNotifier jobCompleteNotifier{};
    unsigned taskId;

    std::atomic< State >  state{ State::Constructed };
};

JobTask::JobTask( unsigned theTaskId )
  : pImple{ new Imple{ theTaskId } }
{
}

JobTask::~JobTask()
{
    delete pImple;
}

void JobTask::activate()
{
    pImple->activate();
}

void JobTask::deactivate()
{
    pImple->deactivate();
}

void JobTask::doJob( JobDataPtrType && pJobData )
{
    pImple->doJob( std::move( pJobData ) );
}

void JobTask::registerJobCompleteNotifier( JobCompleteNotifier && notifier )
{
    pImple->registerJobCompleteNotifier( std::move( notifier ) );
}

unsigned  JobTask::getTaskId()
{
    // We don't need the overhead of calling into the imple for this. Just grab it.
    return pImple->taskId;
}
