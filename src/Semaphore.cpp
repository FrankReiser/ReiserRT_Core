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
    * @param theInitialCount The initial Semaphore count. This is typically zero for an initially unavailable
    * Semaphore. It is clamped at std::numeric_limits< uint32_t >::max() which is roughly 4 billion (2^32-1).
    * @param theMaxAvailableCount The maximum Semaphore count. Zero indicates that the Semaphore
    * is essentially unbounded up to a maximum of (2^32-1). If non-zero, it is clamped to be no less than that
    * of the clamped initial count.
    */
    explicit Imple( size_t theInitialCount, size_t theMaxAvailableCount )
        : takeConditionVar{}
        , giveConditionVar{}
        , mutex{}
        , availableCount{ theInitialCount > std::numeric_limits< AvailableCountType >::max() ?
            std::numeric_limits< AvailableCountType >::max() : AvailableCountType( theInitialCount ) }
        , maxAvailableCount{ doctorMaxAvailableCount( theMaxAvailableCount, availableCount ) }
        , takePendingCount{ 0 }
        , givePendingCount{ 0 }
        , abortFlag{ false }
    {
#ifdef REISER_RT_HAS_PTHREADS
        // Initialize a condition variable attribute
        pthread_condattr_t attr;
        pthread_condattr_init( &attr );

        // Initialize the condition variables
        pthread_cond_init( &takeConditionVar, &attr );
        pthread_cond_init( &giveConditionVar, &attr );

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

        // Sleep a small amount to allow waiters on conditions to get out of the way.
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

#ifdef REISER_RT_HAS_PTHREADS
        // Destroy the condition variables.
        pthread_cond_destroy( &giveConditionVar );
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

public:
    /**
    * @brief The Take Operation
    *
    * This operation locks the mutex and invokes the _take operation to perform some of the work
    * followed by invoking the _takeNotify operation to wake any potential waiters on the give operation.
    * The mutex is released upon return.
    *
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    */
    inline void take()
    {
        std::unique_lock< Mutex > lock{mutex };
        _take(lock);
        _takeNotify();
    }

    /**
    * @brief The Take Operation with Functor Interface
    *
    * This operation locks the mutex and invokes the _take operation to perform some of the work
    * Afterwards, it attempts to invoke the user provide function object.
    * If the user function object should throw an exception, the available count is restored to its former state.
    * Lastly, it invokes the _takeNotify operation to wake any potential waiters on the give operation.
    * The mutex is unlocked upon return.
    *
    * @param operation This is a reference to a user provided function object to invoke during the context of the internal lock.
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    * @throw The user operation may throw exceptions of unspecified type.
    */
    inline void take( const FunctionType & operation ) {
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

        // Notify potential takers that could be waiting.
        _takeNotify();
    }

    /**
    * @brief The Give Operation
    *
    * This operation locks the mutex and invokes the _giveWait operation which may block if the maxAvailableCount
    * would be exceeded. Afterwards, it invokes the _give operation to perform the rest of the work.
    * The mutex is unlocked upon return.
    *
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    */
    inline void give()
    {
        std::unique_lock< Mutex > lock{mutex };
        _giveWait( lock );
        _give();
    }

    /**
    * @brief The Give Operation with Functor Interface
    *
    * This operation locks the mutex and invokes the _giveWait operation which may block if the maxAvailableCount
    * would be exceeded. Afterwards, it invokes the user provide operation. Should the user operation throw an exception
    * the available count does not get incremented. Only after successfully invoking the user provided operation
    * is the _give operation invoked. The mutex is unlocked upon return or if exception is thrown by the user provide
    * operation.
    *
    * @param operation This is a reference to a user provided function object to invoke during the context of the internal lock.
    * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
    */
    inline void give( const FunctionType & operation )
    {
        std::unique_lock< Mutex > lock{mutex };
        _giveWait( lock );
        operation();
        _give();
    }

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

        // Set abort flag and wake up any and all waiters,
        abortFlag = true;
        if ( givePendingCount )
            pthread_cond_broadcast( &giveConditionVar );
        if ( takePendingCount )
#ifdef REISER_RT_HAS_PTHREADS
            pthread_cond_broadcast( &takeConditionVar );
#else
        giveConditionVar.notify_all();
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
    * @brief The Take Notify Internals
    *
    * This operation will notify at most, one waiting give thread.
    */
    inline void _takeNotify()
    {
        // If we have any pending "givers", we must notify them.
        if ( givePendingCount )
        {
#ifdef REISER_RT_HAS_PTHREADS
            pthread_cond_signal( &giveConditionVar );
#else
            giveConditionVar.notify_one();
#endif
        }
    }

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
    void _take( std::unique_lock< Mutex > & lock )
    {
#ifdef REISER_RT_HAS_PTHREADS
        // The code involved with obtaining this native handle is largely or completely inlined
        // and then significantly optimized away.
        auto mutexNativeHandle = lock.mutex()->native_handle();
#endif
        for (;;)
        {
            // If the abort flag is set, throw a SemaphoreAborted exception.
            if ( abortFlag ) throw SemaphoreAborted{ "Semaphore::Imple::_take: Semaphore Aborted!" };

            // If the available count is greater than zero, decrement it and and escape out.
            // We have "taken" the semaphore.  Waiting is not necessary.
            if ( availableCount > 0 )
            {
                --availableCount;
                break;
            }

            // Else we must wait for a notification.
            ++takePendingCount;
#ifdef REISER_RT_HAS_PTHREADS
            pthread_cond_wait(&takeConditionVar, mutexNativeHandle );
#else
            takeConditionVar.wait( lock, [ this ]{ return abortFlag || availableCount > 0; } );
#endif
            // Awakened with test returning true.
            --takePendingCount;
        }
    }

    /**
    * @brief The Give Wait Internals
    *
    * This operation will block if the maximum available count would be exceeded. We must wait for a take
    * to catch up. It expects the mutex to be locked upon invocation.
    */
    void _giveWait( std::unique_lock< Mutex > & lock )
    {
#ifdef REISER_RT_HAS_PTHREADS
        // The code involved with obtaining this native handle is largely or completely inlined
        // and then significantly optimized away.
        auto mutexNativeHandle = lock.mutex()->native_handle();
#endif
        for (;;)
        {
            // If the abort flag is set, throw a SemaphoreAborted exception.
            if ( abortFlag ) throw SemaphoreAborted{ "Semaphore::Imple::_giveWait: Semaphore Aborted!" };

            // If we have already hit the numeric limits for available count, we cannot give anymore.
            // We will throw an exception.
            if ( std::numeric_limits< AvailableCountType >::max() == availableCount )
                    throw SemaphoreOverflow{ "Semaphore::Imple::_giveWait: Absolute Available Count Limit Hit!" };

            // If we can avert a wait, we will do so.
            if ( maxAvailableCount > availableCount )
                break;

            // If here, we have to wait until we can "give" the Semaphore
            ++givePendingCount;

#ifdef REISER_RT_HAS_PTHREADS
            pthread_cond_wait( &giveConditionVar, mutexNativeHandle );
#else
            giveConditionVar.wait( lock, [ this ]{ return abortFlag || availableCount > 0; } );
#endif
            --givePendingCount;
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
        // If the abort flag is set, throw a SemaphoreAborted exception.
        if ( abortFlag ) throw SemaphoreAborted{ "Semaphore::Imple::_giveWait: Semaphore Aborted!" };

        ++availableCount;

#ifdef REISER_RT_HAS_PTHREADS
        pthread_cond_signal( &takeConditionVar );
#else
        takeConditionVar.notify_one();
#endif
    }

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
    * @brief The Static Doctor Max Available Count Operation
    *
    * This operation is provided to assist construction of the Semaphore maxAvailableCount attribute
    * as it was a bit out of hand for simple ternary type logic. It's purpose is to clamp the maxAvailableCount
    * to the absolute maximum limit of (2^32-1) in unbounded mode (input as zero). If non-zero, then to clamp
    * it to be no less than that of the already clamped initialCount value.
    *
    * @param theMaxAvailableCount The maximum available count provided to the Semaphore constructor.
    * @param clampedInitialCount The clamped initial count value determined by the Semaphore constructor.
    *
    * @return The "doctored" maximum available count.
    */
    static AvailableCountType doctorMaxAvailableCount( size_t theMaxAvailableCount,
        AvailableCountType clampedInitialCount )
    {
        if ( theMaxAvailableCount == 0 ) return std::numeric_limits< AvailableCountType >::max();
        if ( theMaxAvailableCount < clampedInitialCount ) return clampedInitialCount;

        // If here, neither of the above were true, return the maximum available count as is.
        return theMaxAvailableCount;
    }

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

    /**
    * @brief The Take Condition Variable
    *
    * This is our take condition variable that we can block on should the available count be zero.
    */
    ConditionVarType takeConditionVar;

    /**
    * @brief The Give Condition Variable
    *
    * This is our give condition variable that we can block on should the available count be at maximum.
    */
    ConditionVarType giveConditionVar;

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
    * @brief The Available Count
    *
    * Our available count is an indication of how many times the take operation can be invoked without
    * blocking, in lieu of any give invocations. The take operation decrements it and the give operation
    * increments it. Once it reaches zero, take operations will block.
    */
    const AvailableCountType maxAvailableCount;

    /**
    * @brief The Take Pending Count
    *
    * This attribute indicates how many threads are pending on our condition variable takeConditionVar
    */
    PendingCountType takePendingCount;

    /**
    * @brief The Give Pending Count
    *
    * This attribute indicates how many threads are pending on our condition variable giveConditionVar
    */
    PendingCountType givePendingCount;

    /**
    * @brief The Abort Flag
    *
    * This attribute indicates that the abort operation has been invoked.
    */
    bool abortFlag;
};

Semaphore::Semaphore( size_t theInitialCount, size_t theMaxAvailableCount )
    : pImple{ new Semaphore::Imple{ theInitialCount, theMaxAvailableCount } }
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

void Semaphore::take( const FunctionType & operation )
{
    pImple->take(operation);
}

void Semaphore::give( )
{
    pImple->give();
}

void Semaphore::give( const FunctionType & operation )
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

