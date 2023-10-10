/**
* @file JobDispatcher.hpp
* @brief The Implementation for some abstract JobDispatcher
* @authors Frank Reiser
* @date Created on June 21, 2021
*/

#include "JobDispatcher.hpp"

#include "ObjectPool.hpp"
#include "JobData.hpp"
#include "JobTask.hpp"
#include "MessageQueue.hpp"
#include "Semaphore.hpp"

//#include <vector>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>
#include <iostream>

class JobDispatcher::Imple
{
private:
    friend class JobDispatcher;

    static constexpr unsigned maxJobs = 128;

    // Let us not make any assumptions about how big an object JobTask is.
    // Therefore, we will use self deleting pointers to them.
    using JobTaskPtrType = std::unique_ptr<JobTask>;

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
        ~BaseMessage() override = default;

        // We do not do anything here. Override or accept the default.
        void dispatch() override {}

        Imple * pImple;	// Need to qualify to disambiguate between MessageQueueBase::Imple
    };

    class TicklerMessage : public BaseMessage
    {
    public:
        using BaseMessage::BaseMessage;
        using BaseMessage::operator=;
    };

    class JobCompleteMessage : public BaseMessage
    {
    public:
        JobCompleteMessage() = delete;
        JobCompleteMessage(Imple* pTheImple, JobDataPtrType && pTheJob )
                : BaseMessage{ pTheImple }
                , pJob{ std::move( pTheJob ) }
        {
        }

        JobCompleteMessage(const JobCompleteMessage & another ) = delete;
        JobCompleteMessage & operator =(const JobCompleteMessage & another ) = delete;

        JobCompleteMessage(JobCompleteMessage && another ) = default;
        JobCompleteMessage & operator =(JobCompleteMessage && another ) = default;

        void dispatch() override { pImple->onJobCompleteMessage(std::move(pJob)) ; }

        JobDataPtrType pJob;
    };

    Imple()
        : pEstTimeGen{ new JobDataEstimatedTimeGenerator }
        , numCPUs( getNumCPUs() )
        , pool{ numCPUs << 1 } // 2x the number of parallel jobs for asynchronous timing slack.
        , messageQueue{ 4, sizeof( JobCompleteMessage ) }
        , completionSemaphore{ 0 }

    {
        // Reserve room to avoid reallocation.
        jobTasks.reserve( numCPUs );

        // We start with 1 since the dispatcher is a thread itself. We will leave CPU zero aside.
        for ( unsigned i=1; numCPUs!=i; ++i)
        {
            // Construct a new JobTask and register our notify job complete callback operation.
            // We will use a lambda function here that captures this, accepts a const JobData reference
            // and invokes our callback. The client need not know any more about us than it looks and sounds like
            // a duck. This is advanced duck typing in action. Of course we could have used some abstract interface
            // but this way creates no additional dependencies. This is taking advantage of compile time polymorphism.
            JobTaskPtrType pJob{new JobTask{i}};
            pJob->registerJobCompleteObserver(
                    [this](JobDataPtrType && pJobData) { notifyJobCompleteCallback( std::move(pJobData) ); }
                    );

            std::cout << "Constructed JobTask #" << i << " and associated our notifyJobCompleteCallback observer."
                << std::endl;

            jobTasks.emplace( i, std::move(pJob) );
        }
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

        delete pEstTimeGen;
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
        msgQueueProcThread = std::thread{ [this]() { messageQueueProc(); } };

        // Activate all my JobTasks.
        for ( auto & iter : jobTasks ) {
            iter.second->activate();
            std::cout << "Activated JobTask #" << iter.second->getTaskId() << "." << std::endl;
        }

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

        // Deactivate all of our JobTasks
        for ( auto & iter : jobTasks )
            iter.second->deactivate();

        // Set state to Constructed and queue up a tickler message to wake our message queue handler.
        state = State::Constructed;
        messageQueue.emplace<TicklerMessage>( this );

        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
    }

    void runJobs()
    {
        // Fire off initial jobs.
        for ( auto & iter : jobTasks )
        {
            auto taskId = iter.first;
            iter.second->doJob(pool.createObj<JobData>(pEstTimeGen, taskId, ++lastJobId ) );
            std::cout << "Fired off Job #" << lastJobId << " to task #" << iter.second->getTaskId() << "." << std::endl;
        }

        // Wait here for completion.
        completionSemaphore.take();
    }

    void onJobCompleteMessage( JobDataPtrType && pJobData )
    {
        // Report that the job has been completed.
        std::cout << "Job #" << pJobData->jobId << " was completed by task #" << pJobData->taskId << ".";
        ++completedJobCount;

        // Any more Jobs left? If so, send off another to the now idle task.
//        if ( maxJobs != ++completedJobCount )
        if ( maxJobs > lastJobId )
        {
            auto & jobTask = jobTasks[ pJobData->taskId ];
            auto taskId = jobTask->getTaskId();
            jobTask->doJob( pool.createObj<JobData>( pEstTimeGen, taskId, ++lastJobId ) );
            std::cout << " Fired off Job #" << lastJobId << " to task #" << jobTask->getTaskId() << "." << std::endl;
        }
        else
            std::cout << std::endl;

        if ( maxJobs == completedJobCount )
            completionSemaphore.give();
    }

    // Message Handling Thread Procedure.
    void messageQueueProc()
    {
#ifdef REISER_RT_HAS_PTHREADS
        // We are assuming a Linux here that supports these non-portable functions being available for this demo.
        const char * taskName = "Dispatch";
        if ( pthread_setname_np( pthread_self(),  taskName ) )
        {
            std::cout << "Thread naming failed for " << taskName << ". Proceeding anyway." << std:: endl;
        }

        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        CPU_SET( 0, &cpuSet );
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

    void notifyJobCompleteCallback( JobDataPtrType && pJobData )
    {
        if ( State::Activated == state )
            messageQueue.emplace<JobCompleteMessage>( this, std::move(pJobData ) );
    }

    static unsigned getNumCPUs()
    {
        return std::thread::hardware_concurrency();
    }

    JobDataEstimatedTimeGenerator * pEstTimeGen;

    // These may be reference and modified by multiple threads so make them atomic.
    std::atomic<unsigned> completedJobCount{ 0 };
    std::atomic<unsigned> lastJobId{ 0 };

    // The number of CPUs we have detected.
    unsigned numCPUs;

    // Maintain boundary alignment due numCPUs above upsetting it. It sort of needs to be first.
    alignas(void *) ReiserRT::Core::ObjectPool< JobData > pool;

    // Our message Queue
    ReiserRT::Core::MessageQueue messageQueue;

    // Our Completion Semaphore
    ReiserRT::Core::Semaphore completionSemaphore;

    // Default construction for these is this. The thread does not start until I move a function into it.
    std::thread msgQueueProcThread{};

    // Our job task objects
    std::unordered_map< unsigned, JobTaskPtrType > jobTasks{};

    // Our behavioral state.
    std::atomic< State > state{ State::Constructed };
};

JobDispatcher::JobDispatcher()
    : pImple{ new Imple }
{
}

JobDispatcher::~JobDispatcher()
{
    delete pImple;
}

void JobDispatcher::activate()
{
    pImple->activate();
}

void JobDispatcher::deactivate()
{
    pImple->deactivate();
}

void JobDispatcher::runJobs()
{
    pImple->runJobs();
}
