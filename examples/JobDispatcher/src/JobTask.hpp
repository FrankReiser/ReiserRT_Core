/**
* @file JobTask.hpp
* @brief The Specification file for some abstract JobTask.
* @authors Frank Reiser
* @date Created on June 21, 2021
*/

#ifndef TESTREISERRT_CORE_JOBTASK_HPP
#define TESTREISERRT_CORE_JOBTASK_HPP

#include "JobDataPtrTypeFwd.hpp"

#include <functional>   // For "Duck Type" callback hookup through std::function.

/**
 * @brief The Job Task
 *
 * The JobTask is an "active/reactive" class. It asynchronously runs jobs when instructed to do so
 * on its "active" hidden worker thread. When it completes a job, will notify a client that registered itself
 * to receive such notifications. The "reactive" portion responds to events. This is accomplished by
 * de-queueing events (messages) in its "active" context and responding to such events accordingly which
 * could be dependent on its state. It makes use of ReiserRT::Core::MessageQueue to accomplish its
 * "reactive-ness".
 */
class JobTask
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
     * @brief The Job Complete Notifier Type
     *
     * This is a type alias for a function that accepts an rvalue reference JobDataPtrType argument and returns
     * nothing. It is the type ("duck type") of function required by the registerJobCompleteObserver.
     * The "Observer" for that function must be convertible to a JobCompleteNotifier.
     */
    using JobCompleteNotifier = std::function< void( JobDataPtrType && ) >;

    /**
     * @brief Constructor for our Job Task
     *
     * The constructor instantiates an instance of our hidden implementation from the standard heap and assign that
     * to our pImple attribute.
     */
    explicit JobTask( unsigned theTaskId );

    /**
     * @brief Destructor for our Job Task
     *
     * The destructor destroys our hidden implementation and returns memory to the standard heap.
     */
    ~JobTask();

    /**
     * @brief Generic Observer Job Complete Registration Operation
     *
     * This template operation registers a "duck type" observer for job complete notifications.
     * Registration must be accomplished prior to "activation" via the activate operation.
     *
     * @tparam Observer A function capable of being converted into a JobCompleteNotifier or a
     * JobCompleteNotifier in and of itself or a reference to one. It must look like a duck and hopefully
     * sounds like one too.
     * @param observer An rvalue reference to an instance of Observer (yes move which may be equivalent to copy).
     */
    template< typename Observer >
    void registerJobCompleteObserver( Observer && observer )
    {
        // Validate that the observer supports the required functor signature.
        static_assert( std::is_constructible< JobCompleteNotifier, Observer >::value,
                       "JobCompleteNotifier cannot be constructed from template argument 'Observer'!" );

        // Delegate to implementation.
        registerJobCompleteNotifier( JobCompleteNotifier{ std::forward< Observer >( observer ) } );
    }

    /**
     * @brief The Activate Operation
     *
     * This operation starts the worker thread and sets the JobTask to the "Activated" state.
     * JobTask will start its "reactive" processing of events (messages).
     */
    void activate();

    /**
     * @brief The Deactivate Operation
     *
     * This operation stops the "reactive" processing of events.
     */
    void deactivate();

    /**
     * @brief The Do Job Operation
     *
     * This operation receives a job, wraps it up in a message and enqueues that message
     * into the internal message queue for asynchronous processing by the "active" context.
     * It will be de-queued and "reacted" to (do the job). If a JobCompleteNotifier was registered,
     * it will be invoked when the job has completed,
     *
     * @param pJobData An rvalue reference to the JobDataPtrType instance that will be moved into the operation.
     */
    void doJob( JobDataPtrType && pJobData );

    /**
     * @brief The Get Task Id Operation
     *
     * This operation simply returns the task identifier we were instantiated with.
     *
     * @return Returns the task identifier we were instantiated with.
     */
    unsigned getTaskId();

private:
    /**
     * @brief Observer Job Complete Registration Helper Operation
     *
     * This operation is used by the registerJobCompleteObserver after the generic observer
     * has been converted to a JobCompleteNotifier by the compiler.
     *
     * @param notifier An rvalue reference to an instance of a JobCompleteNotifier.
     */
    void registerJobCompleteNotifier( JobCompleteNotifier && notifier );

private:
    /**
     * @brief Pointer to Hidden Implementation
     *
     * This attribute will point to our hidden implementation after construction.
     */
    Imple * pImple;
};


#endif //TESTREISERRT_CORE_JOBTASK_HPP
