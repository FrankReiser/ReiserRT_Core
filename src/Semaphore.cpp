/**
* @file Semaphore.cpp
* @brief The Implementation for Semaphore utility
* @authors: Frank Reiser
* @date Created on Jul 8, 2015
*/

//#include "ProjectConfigure.h"

#include "Semaphore.hpp"

#include "Mutex.hpp"

#include "ReiserRT_CoreExceptions.hpp"

#include <limits>
#include <cstdint>
#ifndef REISER_RT_HAS_PTHREADS
#include <condition_variable>
#endif
#include <mutex>
#include <thread>


using namespace ReiserRT::Core;

/**
* @brief A Counted, Wait-able Semaphore Class
*
* This class provides the implementation specifics for the Semaphore class.
*/
class ReiserRT_Core_EXPORT Semaphore::Imple
{
public:
    /**
    * @brief Qualified Constructor for Implementation
    *
    * This operation constructs the Implementation.
    *
    * @param theInitialCount The initial Semaphore count, defaults to zero and is clamped to
    * std::numeric_limits< AvailableCountType>::max() or slightly more than 4 billion.
    */
    explicit Imple( size_t theInitialCount = 0 );

    /**
    * @brief Destructor for the Implementation
    *
    * This destructor invokes the abort operation. If the implementation is still being
    * used by any thread when this destructor is invoked, those threads could experience
    * ReiserRT::Core::SemaphoreAborted being thrown.
    */
    ~Imple();

    /**
    * @brief Copy Constructor for Implementation
    *
    * Copying the implementation is disallowed. Hence, this operation has been deleted.
    *
    * @param another Another instance of the implementation.
    */
    Imple( const Imple & another ) = delete;

    /**
    * @brief Copy Assignment Operation for Implementation
    *
    * Copying the implementation is disallowed. Hence, this operation has been deleted.
    *
    * @param another Another instance of the implementation.
    */
    Imple & operator =( const Imple & another ) = delete;

    /**
    * @brief Move Constructor for Implementation
    *
    * Moving the implementation is disallowed. Hence, this operation has been deleted.
    *
    * @param another An rvalue reference to another instance of the implementation.
    */
    Imple( Imple && another ) = delete;

    /**
    * @brief Move Assignment Operation for Implementation
    *
    * Moving the implementation is disallowed. Hence, this operation has been deleted.
    *
    * @param another An rvalue reference to another instance of the implementation.
    */
    Imple & operator =( Imple && another ) = delete;

    /**
    * @brief Available 32bit Count Type
    *
    * We use a signed 32bit unsigned integer for our available counter. This is large enough
    * to track over four billion of whatever resource.
    */
    using AvailableCountType = uint32_t;

    /**
    * @brief Pending 16bit Count Type
    *
    * We use a signed 16bit integer for our pending counter. There can only be as
    * many pending clients as there are threads entering the wait operation
    * and actually waiting.
    */
    using PendingCountType = uint16_t;

    /**
    * @brief Condition Variable Type
    *
    * The type of condition variable we are using.
    */
#ifdef REISER_RT_HAS_PTHREADS
    using ConditionVarType = pthread_cond_t;
#else
    using ConditionVarType = std::condition_variable_any;
#endif

public:
    /**
    * @brief The Wait Operation
    *
    * This operation takes the mutex and invokes the _wait operation to perform the rest of the work.
    * The mutex is released upon return.
    *
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    */
    inline void wait() { std::unique_lock< Mutex > lock{ mutex }; _wait( lock ); }

    /**
    * @brief The Wait Operation
    *
    * This operation takes the mutex and invokes the _wait operation to perform the rest of the waiting work.
    * After the wait work has been done, it attempts to invokes the user provide function object.
    * If the user function object should throw an exception, the available count is restored to its former state.
    * The mutex is released upon return.
    *
    * @param operation This is a reference to a user provided function object to invoke during the context of the internal lock.
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    * @throw The user operation may throw an exception of unknown type.
    */
    inline void wait( FunctionType & operation ) {
        std::unique_lock< Mutex > lock{ mutex };
        _wait( lock );
#if 1
        struct AvailableCountManager {
            AvailableCountManager( AvailableCountType & theAC ) : rAC(theAC) {}
            ~AvailableCountManager() { if ( !released ) ++rAC; }

            void release() { released = true; }

            AvailableCountType & rAC;
            bool released{ false };
        };

        // Guard the available count and call user provided operation.
        AvailableCountManager availableCountManager{ availableCount };
        operation();
        availableCountManager.release();
#else
        try {
            operation();
        }
        catch ( std::exception & )
        {
            ++availableCount;
            throw;
        }
#endif
    }

    /**
    * @brief The Notify Operation
    *
    * This operation takes the mutex and invokes the _notify operation to perform the rest of the work.
    * The mutex is released upon return.
    *
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    */
    inline void notify() { std::lock_guard< Mutex > lock{ mutex }; _notify(); }

    /**
    * @brief The Notify Operation with Functor Interface
    *
    * This operation takes the mutex and invokes the user provide function object prior to invoking the _notify operation
    * to perform the rest of the work.
    * The mutex is released upon return.
    *
    * @param operation This is a reference to a user provided function object to invoke during the context of the internal lock.
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    */
    inline void notify( FunctionType & operation ) { std::lock_guard< Mutex > lock{ mutex }; operation(); _notify(); }

    /**
    * @brief The Abort Operation
    *
    * This operation, briefly takes the mutex and tests whether we are already aborted and if so, releases and returns.
    * Otherwise, sets the abortFlag and invokes the conditionVal, notify_all operation to wake all waiting threads.
    * The mutex is released upon return.
    */
    void abort();

    /**
    * @brief Get Available Count
    *
    * This is primarily an implementation validation operation. It returns a snapshot of the
    * current available count. The mutex is briefly taken and a copy of availableCount is returned
    * as the mutex is released.
    *
    * @return Returns a snapshot of the availableCount at time of invocation.
    */
    size_t getAvailableCount();

private:
    /**
    * @brief The Wait Operation Internals
    *
    * This operation is for internal use. It expects that our mutex has been acquired prior to invocation.
    * The operation attempts to decrement the availableCount towards zero and then returns.
    * If the availableCount is zero, the pendingCount is incremented and then the call blocks on our conditionVar simultaneously
    * unlocking the mutex until notified to unblock.
    * Once notified, the mutex is re-taken, the pendingCount is decremented and we re-loop attempting to
    * decrement the availableCount towards zero once more.
    *
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abortFlag has been set via the abort operation.
    */
    void _wait( std::unique_lock< Mutex > & lock );

    /**
    * @brief The Notify Operation Internals
    *
    * This operation is for internal use. It expects that our mutex has been acquired prior to invocation.
    * The operation increments the availableCount and if pendingCount is
    * greater than zero, invokes the conditionVar, notify_one operation to wake one waiting thread.
    *
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abortFlag has been set via the abort operation.
    */
    void _notify();

    /**
    * @brief The Condition Variable
    *
    * This is an instance of a C++11 std::condition_variable. On Linux systems, it is a light weight
    * wrapper around a POSIX condition variable and functions as one would expect. Condition variables
    * are design to allow threads of execution to efficiently wait (block) until a "condition" becomes true.
    * The operate in conjunction with a mutex.
    */
    ConditionVarType conditionVar;

    /**
    * @brief The Mutex
    *
    * This contains an instance of a POSIX mutex on Linux systems.
    * Mutexes are designed to provide mutual exclusion around sections of code to ensure coherency
    * of data accessed in multi-threaded environments.
    */
    Mutex mutex;

    /**
    * @brief The Available Count
    *
    * Our available count is an indication of how many times the wait operation can be invoked without
    * blocking, in lieu of any notify invocations. The wait operation decrements it and the notify operation
    * increments it. Once it reaches zero, wait operations will block.
    */
    AvailableCountType availableCount;

    /**
    * @brief The PendingCount
    *
    * This attribute indicates how many threads are pending on our condition variable conditionVar
    */
    PendingCountType pendingCount;

    /**
    * @brief The Abort Flag
    *
    * This attribute indicates that the abort operation has been invoked.
    */
    bool abortFlag;
};


Semaphore::Imple::Imple( size_t theInitialCount )
    : conditionVar{}
    , mutex{}
    , availableCount{ theInitialCount > std::numeric_limits< AvailableCountType >::max() ?
                    std::numeric_limits< AvailableCountType >::max() : AvailableCountType( theInitialCount ) }
    , pendingCount{ 0 }
    , abortFlag{ false }
{
#ifdef REISER_RT_HAS_PTHREADS
    // Initialize a condition variable attribute
    pthread_condattr_t attr;
    pthread_condattr_init( &attr );

    // Initialize condition variable
    pthread_cond_init( &conditionVar, &attr );

    // Destroy attribute, we are done with it.
    pthread_condattr_destroy( &attr );
#endif
}

Semaphore::Imple::~Imple()
{
    abort();

    // Sleep a small amount to allow waiters to get out of the way.
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

#ifdef REISER_RT_HAS_PTHREADS
    // Destroy the condition variable.
    pthread_cond_destroy( &conditionVar );
#endif
}

void Semaphore::Imple::_wait( std::unique_lock< Mutex > & lock )
{
#ifdef REISER_RT_HAS_PTHREADS
    // The code involved with obtaining this native handle is largely or completely inlined
    // and then significantly optimized away.
    auto mutexNativeHandle = lock.mutex()->native_handle();
#endif
    for (;;)
    {
        // If the abort flag is set, throw a SemaphoreAbortedException.
        if ( abortFlag ) throw SemaphoreAborted{ "Semaphore::Imple::_wait: Semaphore Aborted!" };

        // If the available count is greater than zero, decrement it and and escape out.
        // We have "taken" the semaphore.  Waiting is not necessary.
        if ( availableCount > 0 )
        {
            --availableCount;
            return;
        }

        // Else we must wait for a notification.
        ++pendingCount;
#ifdef REISER_RT_HAS_PTHREADS
        pthread_cond_wait( &conditionVar, mutexNativeHandle );
#else
        conditionVar.wait( lock, [ this ]{ return abortFlag || availableCount > 0; } );
#endif
        // Awakened with test returning true.
        --pendingCount;
    }
}

void Semaphore::Imple::_notify()
{
    // If the abort flag is set OR we have been "over notified", throw a runtime error.
    if ( abortFlag || availableCount == std::numeric_limits< AvailableCountType >::max() ) {
        throw SemaphoreAborted{ abortFlag ?
            "Semaphore::Imple::_notify: Semaphore Aborted!" : "Semaphore::Imple::_notify: Notification Limit Hit!" };
    }

    ++availableCount;

#ifdef REISER_RT_HAS_PTHREADS
    pthread_cond_signal( &conditionVar );
#else
    conditionVar.notify_one();
#endif
}

void Semaphore::Imple::abort()
{
    std::lock_guard< Mutex > lock( mutex );

    // If the abort flag is set, quietly get out of the way.
    if ( abortFlag ) return;

    abortFlag = true;
    if ( pendingCount != 0 )
#ifdef REISER_RT_HAS_PTHREADS
        pthread_cond_broadcast( &conditionVar );
#else
        conditionVar.notify_all();
#endif
}

size_t Semaphore::Imple::getAvailableCount()
{
    std::lock_guard< Mutex > lock( mutex );

    return availableCount;
}

Semaphore::Semaphore( size_t theInitialCount )
    : pImple{ new Semaphore::Imple{ theInitialCount } }
{
}

Semaphore::~Semaphore()
{
    delete pImple;
}

void Semaphore::wait()
{
    pImple->wait();
}

void Semaphore::wait( FunctionType operation )
{
    pImple->wait( operation );
}

void Semaphore::notify( )
{
    pImple->notify();
}

void Semaphore::notify( FunctionType operation )
{
    pImple->notify( operation );
}

void Semaphore::abort()
{
    pImple->abort();
}

size_t Semaphore::getAvailableCount()
{
    return pImple->getAvailableCount();
}

