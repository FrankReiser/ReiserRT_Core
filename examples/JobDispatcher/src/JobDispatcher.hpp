/**
* @file JobDispatcher.hpp
* @brief The Specification for some abstract JobDispatcher
* @authors Frank Reiser
* @date Created on June 21, 2021
*/

#ifndef TESTREISERRT_CORE_JOBDISPATCHER_HPP
#define TESTREISERRT_CORE_JOBDISPATCHER_HPP

/**
 * @brief The Job Dispatcher
 *
 * The JobDispatcher is an "active/reactive" class. When "activated" and subsequently "run"
 * it will fire off an initial number of jobs based on the number of CPUs available on the system (less one).
 * It then asynchronously responds to job completion events (messages) and will immediately deliver another
 * job to the task that issued the completion event. This process continues until there are no jobs left
 * to deliver. It then waits for any remaining jobs to complete.
 *
 * JobDispatcher instantiates a number JobTask based on the number of CPUs available on the system (less one).
 * It also makes use of ReiserRT::Core::ObjectPool for raw memory on which JobData is allocated. Pointers returned
 * from ObjectPool can be moved around safely and efficiently. It also makes use of ReiserRT::Core::MessageQueue
 * to accomplish its "reactive-ness" which is to react to job complete notifications. It also makes use of
 * ReiserRT::Core::Semaphore for signaling that all jobs are completed which the runJobs operation will be
 * will be waiting for.
 */
class JobDispatcher
{
private:
    /**
     * @brief Forward Declaration of Hidden Implementation.
     *
     * Along the lines of the PIMPLE idiom (pointer to hidden implementation), we are going to hide our
     * our implementation details.
     * Rationale is that we do not to create dependencies for client's use of our interface if we can avoid it.
     */
    class Imple;

public:
    /**
     * @brief Constructor for our Job Dispatcher
     *
     * The constructor instantiates an instance of our hidden implementation from the standard heap and assign that
     * to our pImple attribute.
     */
    JobDispatcher();

    /**
     * @brief Destructor for our Job Dispatcher
     *
     * The destructor destroys our hidden implementation and returns memory to the standard heap.
     */
    ~JobDispatcher();

    /**
     * @brief The Activate Operation
     *
     * This operation starts the worker thread and sets the JobDispatcher to the "Activated" state.
     * JobDispatcher will start its "reactive" processing of events (messages).
     */
    void activate();

    /**
     * @brief The Deactivate Operation
     *
     * This operation stops the "reactive" processing of events.
     */
    void deactivate();

    /**
     * @brief The Run Jobs Operation
     *
     * This "synchronous" operation will fire off the initial "do job" request to the 'N' JobTask
     * instances. It will then wait until all jobs are complete before returning.
     */
    void runJobs();

private:
    /**
     * @brief Pointer to Hidden Implementation
     *
     * This attribute will point to our hidden implementation after construction.
     */
    Imple * pImple;
};

#endif //TESTREISERRT_CORE_JOBDISPATCHER_HPP
