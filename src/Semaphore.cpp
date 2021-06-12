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
    explicit Imple( size_t theInitialCount = 0 )
        : takeConditionVar{}
        , mutex{}
        , availableCount{ theInitialCount > std::numeric_limits< AvailableCountType >::max() ?
                          std::numeric_limits< AvailableCountType >::max() : AvailableCountType( theInitialCount ) }
        , takePendingCount{0 }
        , abortFlag{ false }
    {
#ifdef REISER_RT_HAS_PTHREADS
        // Initialize a condition variable attribute
        pthread_condattr_t attr;
        pthread_condattr_init( &attr );

        // Initialize condition variable
        pthread_cond_init(&takeConditionVar, &attr );

        // Destroy attribute, we are done with it.
        pthread_condattr_destroy( &attr );
#endif
    }

    /**
    * @brief Destructor for the Implementation
    *
    * This destructor invokes the abort operation. If the implementation is still being
    * used by any thread when this destructor is invoked, those threads could experience
    * ReiserRT::Core::SemaphoreAborted being thrown.
    */
    ~Imple()
    {
        abort();

        // Sleep a small amount to allow waiters to get out of the way.
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

#ifdef REISER_RT_HAS_PTHREADS
        // Destroy the condition variable.
        pthread_cond_destroy( &takeConditionVar );
#endif
    }

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
    * many pending clients as there are threads entering the take operation
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
    * @brief The Take Operation
    *
    * This operation locks the mutex and invokes the _take operation to perform the rest of the work.
    * The mutex is released upon return.
    *
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    */
    inline void take() { std::unique_lock< Mutex > lock{mutex };
        _take(lock); }

    /**
    * @brief The Take Operation with Functor Interface
    *
    * This operation locks our mutex and invokes the _take operation to perform the rest of the work.
    * After the additional work has been done, it attempts to invokes the user provide function object.
    * If the user function object should throw an exception, the available count is restored to its former state.
    * The mutex is unlocked upon return.
    *
    * @param operation This is a reference to a user provided function object to invoke during the context of the internal lock.
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    * @throw The user operation may throw exceptions of unspecified type.
    */
    inline void take(FunctionType & operation ) {
        std::unique_lock< Mutex > lock{ mutex };
        _take(lock);

        struct AvailableCountManager {
            explicit AvailableCountManager( AvailableCountType & theAC ) noexcept : rAC(theAC)  {}
            ~AvailableCountManager() noexcept { if ( !released ) ++rAC; }

            void release() { released = true; }

            AvailableCountType & rAC;
            bool released{ false };
        };

        // Guard the available count and call user provided operation.
        AvailableCountManager availableCountManager{ availableCount };
        operation();
        availableCountManager.release();
    }

    /**
    * @brief The Give Operation
    *
    * This operation locks the mutex and invokes the _give operation to perform the rest of the work.
    * The mutex is unlocked upon return.
    *
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    */
    inline void give() { std::lock_guard< Mutex > lock{mutex };
        _give(); }

    /**
    * @brief The Give Operation with Functor Interface
    *
    * This operation locks the mutex and invokes the user provide function object prior to invoking the _give operation
    * to perform the rest of the work.
    * The mutex is unlocked upon return.
    *
    * @param operation This is a reference to a user provided function object to invoke during the context of the internal lock.
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    */
    inline void give(FunctionType & operation ) { std::lock_guard< Mutex > lock{mutex }; operation();
        _give(); }

    /**
    * @brief The Abort Operation
    *
    * This operation, briefly takes the mutex and tests whether we are already aborted and if so, releases and returns.
    * Otherwise, sets the abortFlag and invokes the conditionVal, notify_all operation to wake all waiting threads.
    * The mutex is released upon return.
    */
    void abort()
    {
        std::lock_guard< Mutex > lock( mutex );

        // If the abort flag is set, quietly get out of the way.
        if ( abortFlag ) return;

        abortFlag = true;
        if (takePendingCount != 0 )
#ifdef REISER_RT_HAS_PTHREADS
            pthread_cond_broadcast( &takeConditionVar );
#else
        takeConditionVar.notify_all();
#endif
    }

    /**
    * @brief Get Available Count
    *
    * This is primarily an implementation validation operation. It returns a snapshot of the
    * current available count. The mutex is briefly taken and a copy of availableCount is returned
    * as the mutex is released.
    *
    * @return Returns a snapshot of the availableCount at time of invocation.
    */
    size_t getAvailableCount()
    {
        std::lock_guard< Mutex > lock( mutex );

        return availableCount;
    }

private:
    /**
    * @brief The Take Operation Internals
    *
    * This operation is for internal use by the outer "take" operations.
    * It expects that our mutex has been acquired prior to invocation.
    * The operation attempts to decrement the availableCount towards zero and then returns.
    * If the availableCount is zero, the takePendingCount is incremented and then the call blocks on our
    * takeConditionVar simultaneously unlocking the mutex until notified to unblock.
    * Once notified, the mutex is re-locked, the takePendingCount is decremented and we re-loop attempting to
    * decrement the availableCount towards zero once more.
    *
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abortFlag has been set via the abort operation.
    */
    void _take(std::unique_lock< Mutex > & lock )
    {
#ifdef REISER_RT_HAS_PTHREADS
        // The code involved with obtaining this native handle is largely or completely inlined
        // and then significantly optimized away.
        auto mutexNativeHandle = lock.mutex()->native_handle();
#endif
        for (;;)
        {
            // If the abort flag is set, throw a SemaphoreAbortedException.
            if ( abortFlag ) throw SemaphoreAborted{ "Semaphore::Imple::_take: Semaphore Aborted!" };

            // If the available count is greater than zero, decrement it and and escape out.
            // We have "taken" the semaphore.  Waiting is not necessary.
            if ( availableCount > 0 )
            {
                --availableCount;
                return;
            }

            // Else we must take for a notification.
            ++takePendingCount;
#ifdef REISER_RT_HAS_PTHREADS
            pthread_cond_wait(&takeConditionVar, mutexNativeHandle );
#else
            takeConditionVar.take( lock, [ this ]{ return abortFlag || availableCount > 0; } );
#endif
            // Awakened with test returning true.
            --takePendingCount;
        }
    }

    /**
    * @brief The Give Operation Internals
    *
    * This operation is for internal use by the outer "give" operations.
    * It expects that our mutex has been acquired prior to invocation.
    * The operation increments the availableCount and if takePendingCount is
    * greater than zero, invokes the takeConditionVar, notify_one operation to wake one waiting thread.
    *
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abortFlag has been set via the abort operation.
    */
    void _give()
    {
        // If the abort flag is set OR we have been "over notified", throw a runtime error.
        if ( abortFlag || availableCount == std::numeric_limits< AvailableCountType >::max() ) {
            throw SemaphoreAborted{ abortFlag ? "Semaphore::Imple::_give: Semaphore Aborted!" :
                "Semaphore::Imple::_give: Notification Limit Hit!" };
        }

        ++availableCount;

#ifdef REISER_RT_HAS_PTHREADS
        pthread_cond_signal( &takeConditionVar );
#else
        takeConditionVar.notify_one();
#endif
    }

    /**
    * @brief The Take Condition Variable
    *
    * This is our take condition variable that we can block on should the available count be zero.
    */
    ConditionVarType takeConditionVar;

    /**
    * @brief The Mutex
    *
    * This is our mutual exclusion object used to synchronize access to our state attributes.
    */
    Mutex mutex;

    /**
    * @brief The Available Count
    *
    * Our available count is an indication of how many times the take operation can be invoked without
    * blocking, in lieu of any give invocations. The take operation decrements it and the give operation
    * increments it. Once it reaches zero, take operations will block.
    */
    AvailableCountType availableCount;

    /**
    * @brief The Take Pending Count
    *
    * This attribute indicates how many threads are pending on our condition variable takeConditionVar
    */
    PendingCountType takePendingCount;

    /**
    * @brief The Abort Flag
    *
    * This attribute indicates that the abort operation has been invoked.
    */
    bool abortFlag;
};

Semaphore::Semaphore( size_t theInitialCount )
    : pImple{ new Semaphore::Imple{ theInitialCount } }
{
}

Semaphore::~Semaphore()
{
    delete pImple;
}

void Semaphore::take()
{
    pImple->take();
}

void Semaphore::take(FunctionType operation )
{
    pImple->take(operation);
}

void Semaphore::give( )
{
    pImple->give();
}

void Semaphore::give(FunctionType operation )
{
    pImple->give(operation);
}

void Semaphore::abort()
{
    pImple->abort();
}

size_t Semaphore::getAvailableCount()
{
    return pImple->getAvailableCount();
}

