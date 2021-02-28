/**
* @file Semaphore.cpp
* @brief The Implementation for Semaphore utility
* @authors: Frank Reiser
* @date Created on Jul 8, 2015
*/

//#include "ProjectConfigure.h"

//#define SEMAPHORE_USES_PRIORITY_INHERIT_MUTEX 1 ///@todo This should be obtained from platform detection.

#include "ReiserRT_Core/Semaphore.hpp"

#ifdef REISER_RT_HAS_PTHREADS
#include "PriorityInheritMutex.hpp"
#endif

#include <limits>
#include <cstdint>
#include <condition_variable>
#include <mutex>


using namespace ReiserRT::Core;

/// @todo Document class Semaphore::Imple
class Semaphore::Imple
{
public:
    /**
    * @brief Qualified Constructor for Implementation
    *
    * This operation constructs tImplementation.
    *
    * @param theInitialCount The initial Semaphore count, defaults to zero and is clamped to
    * std::numeric_limits< AvailableCountType>::max() or slightly more than 2 billion.
    */
    explicit Imple( size_t theInitialCount = 0 );

    /**
    * @brief Destructor for the Implementation
    *
    * This destructor invokes the abort operation. If the implementation is still being
    * used by any thread when this destructor is invoked, those threads could experience runtime_error being thrown.
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
    * This type provides a little "syntactic sugar" for the class.
    */
#ifdef REISER_RT_HAS_PTHREADS
    using ConditionVarType = std::condition_variable_any;
#else
    using ConditionVarType = std::condition_variable;
#endif

    /**
    * @brief Mutex Type
    *
    * This type provides a little "syntactic sugar" for the class.
    */
#ifdef REISER_RT_HAS_PTHREADS
    using MutexType = PriorityInheritMutex;
#else
    using MutexType = std::mutex;
#endif

public:
    /**
    * @brief The Wait Operation
    *
    * This operation takes the mutex and invokes the _wait operation to perform the rest of the work.
    * The mutex is released upon return.
    *
    * @throw Throws std::runtime_error if the abort operation is invoked via another thread.
    */
    inline void wait() { std::unique_lock< MutexType > lock{ mutex }; _wait( lock ); }

    /**
    * @brief The Wait Operation
    *
    * This operation takes the mutex and invokes the _wait operation to perform the rest of the waiting work.
    * After the wait work has been done, it invokes the user provide function object.
    * The mutex is released upon return.
    *
    * @param operation This is a reference to a user provided function object to invoke during the context of the internal lock.
    * @throw Throws std::runtime_error if the abort operation is invoked via another thread.
    */
    inline void wait( FunctionType & operation ) { std::unique_lock< MutexType > lock{ mutex }; _wait( lock ); operation(); }

    /**
    * @brief The Notify Operation
    *
    * This operation takes the mutex and invokes the _notify operation to perform the rest of the work.
    * The mutex is released upon return.
    *
    * @throw Throws std::runtime_error if the abort operation is invoked via another thread.
    */
    inline void notify() { std::lock_guard< MutexType > lock{ mutex }; _notify(); }

    /**
    * @brief The Notify Operation with Functor Interface
    *
    * This operation takes the mutex and invokes the user provide function object prior to invoking the _notify operation
    * to perform the rest of the work.
    * The mutex is released upon return.
    *
    * @param operation This is a reference to a user provided function object to invoke during the context of the internal lock.
    * @throw Throws std::runtime_error if the abort operation is invoked via another thread.
    */
    inline void notify( FunctionType & operation ) { std::lock_guard< MutexType > lock{ mutex }; operation(); _notify(); }

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
    * @throw Throws std::runtime_error if the abortFlag has been set via the abort operation.
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
    * @throw Throws std::runtime_error if the abortFlag has been set via the abort operation.
    */
    void _wait( std::unique_lock< MutexType > & lock );

    /**
    * @brief The Notify Operation Internals
    *
    * This operation is for internal use. It expects that our mutex has been acquired prior to invocation.
    * The operation increments the availableCount and if pendingCount is
    * greater than zero, invokes the conditionVar, notify_one operation to wake one waiting thread.
    *
    * @throw Throws std::runtime_error if the abortFlag has been set via the abort operation.
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
    MutexType mutex;

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
}

Semaphore::Imple::~Imple()
{
    abort();
}


void Semaphore::Imple::_wait( std::unique_lock< MutexType > & lock )
{
    for (;;)
    {
        // If the abort flag is set, throw a SemaphoreAbortedException.
        if ( abortFlag ) throw std::runtime_error{ "Semaphore::Imple::_wait: Semaphore Aborted!" };

        // If the available count is greater than zero, decrement it and and escape out.
        // We have "taken" the semaphore.  Waiting is not necessary.
        if ( availableCount > 0 )
        {
            --availableCount;
            return;
        }

        // Else we must wait for a notification.
        ++pendingCount;
        conditionVar.wait( lock, [ this ]{ return abortFlag || availableCount > 0; } );

        // Awakened with test returning true.
        --pendingCount;
    }
}

void Semaphore::Imple::_notify()
{
    // If the abort flag is set, throw a SemaphoreAbortedException.
    if ( abortFlag ) throw std::runtime_error{ "Semaphore::Imple::_notify: Semaphore Aborted!" };

    ++availableCount;

    conditionVar.notify_one();
}

void Semaphore::Imple::abort()
{
    std::lock_guard< MutexType > lock( mutex );

    // If the abort flag is set, quietly get out of the way.
    if ( abortFlag ) return;

    abortFlag = true;
    if ( pendingCount != 0 )
        conditionVar.notify_all();
}

size_t Semaphore::Imple::getAvailableCount()
{
    std::lock_guard< MutexType > lock( mutex );

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

