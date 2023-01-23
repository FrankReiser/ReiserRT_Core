/**
* @file Semaphore.hpp
* @brief The Specification for Semaphore utility
* @authors: Frank Reiser
* @date Created on May 29, 2015
*/

#ifndef REISERRT_CORE_SEMAPHORE_HPP
#define REISERRT_CORE_SEMAPHORE_HPP

#include "ReiserRT_CoreExport.h"

#include <functional>
#include <cstddef>

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief A Counted, Wait-able Semaphore Class
        *
        * This class provides a implementation of a thread-safe, take-able counted semaphore similar to Unix System V
        * semaphores which are not part of the C++11 pr POSIX standards. Counted Semaphores are a resource management tool.
        * They are designed to efficiently manage resource availability, until resources are exhausted (count of zero)
        * at which point, a client must take (block) until resources are released and made available for reuse.
        *
        * Our Semaphore supports two modes of operation. This is determined at construction time.
        * The first mode is that of unbounded give operations up to an absolute limit of 2^32-1 at which point an
        * exception is thrown. The second mode is that of bounded give operations up to a specified maximum at which
        * point blocking occurs until a take is initiated. This second mode is referred to a bipolar operation.
        * This is perhaps unnatural but turns out to be extremely useful in producer/consumer patterns.
        *
        * @todo Support timed give and take operations? Maybe some day.
        */
        class ReiserRT_Core_EXPORT Semaphore
        {
        private:
            /**
            * @brief Forward Declaration of Hidden Implementation.
            *
            * The Semaphore class hides its implementation details by employing the "pImple" idiom.
            */
            class Imple;

        public:
            /**
            * @brief The Function Type for give and take operations that accept a functor interface.
            *
            * This is the type of functor expected when using give or take operations
            * that accept a user provided functor to invoke while a lock is held. This should be a relatively
            * simple operation in order to keep lock duration period to a minimum. These should be non-blocking
            * operations. See give and take documentation for the operations that accept functor arguments for
            * additional details.
            */
            using FunctionType = std::function< void() >;

            /**
            * @brief Qualified Constructor for Semaphore
            *
            * This operation constructs a Semaphore.
            *
            * @param theInitialCount The initial Semaphore count. This is typically zero for an initially unavailable
            * Semaphore. It is clamped at std::numeric_limits< uint32_t >::max() which is roughly 4 billion (2^32-1).
            * @param theMaxAvailableCount The maximum Semaphore count. Zero indicates that the Semaphore
            * is essentially unbounded up to a maximum of (2^32-1). If non-zero, it is clamped to be no less than that
            * of the clamped initial count.
            * @note A non-zero value of theMaxAvailableCount specifies that the Semaphore operate in bipolar mode.
            * In essence, give operations will block if the available count would exceed the maximum specified.
            */
            explicit Semaphore( size_t theInitialCount, size_t theMaxAvailableCount = 0 );

            /**
            * @brief Destructor for the Semaphore
            *
            * This destructor invokes the abort operation. If the Semaphore is still being
            * used by any thread when this destructor is invoked, those threads will experience a runtime error.
            */
            ~Semaphore();

            /**
            * @brief Copy Constructor for Semaphore
            *
            * Copying Semaphore is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a Semaphore.
            */
            Semaphore( const Semaphore & another ) = delete;

            /**
            * @brief Copy Assignment Operation for Semaphore
            *
            * Copying Semaphore is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a Semaphore.
            */
            Semaphore & operator =( const Semaphore & another ) = delete;

            /**
            * @brief Move Constructor for Semaphore
            *
            * Moving Semaphore is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a Semaphore.
            */
            Semaphore( Semaphore && another ) = delete;

            /**
            * @brief Move Assignment Operation for Semaphore
            *
            * Moving Semaphore is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a Semaphore.
            */
            Semaphore & operator =( Semaphore && another ) = delete;

            /**
            * @brief The Take Operation
            *
            * This operation attempts to decrement the available count towards zero.
            * If the available count is already zero, the operation will block until available count is incremented
            * away from zero.
            *
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
            */
            void take();

            /**
            * @brief The Take Operation with Functor Interface
            *
            * This operation attempts to decrement the availableCount towards zero.
            * If the available count is already zero, the operation will block until availableCount is incremented
            * away from zero. When the take has successfully decremented the available count,
            * the user provided function object is invoked while an internal lock is held.
            * This provides affords the client an opportunity to do bookkeeping under mutual exclusion without having to
            * take a separate lock.
            *
            * @param operation A reference to a user provided function object to be invoked after the availableCount is decremented.
            * The user operation is invoked while an internal lock is held.
            * @warning Should the user operation throw an exception, the available count will be restored to its former state as if
            * the take call was never invoked.
            * @throw Throws std::bad_function_call if the operation passed in has no target (an empty function object).
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread.
            * @throw The user operation may throw an exception of unknown type.
            */
            void take( const FunctionType & operation );

            /**
            * @brief The Give Operation
            *
            * This operation increments the available count away from zero and will wake, at most, one waiting thread.
            * It may block if operating in bipolar mode and the clamped maximum available count would be exceeded
            * prior to incrementing the available count and waking of a thread.
            *
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread or if we have been
            * notified more times than we can count (2^32 -1).
            */
            void give();

            /**
            * @brief The Give Operation with Functor Interface
            *
            * This operation increments the available count away from zero and will wake, at most, one waiting thread.
            * It may block if operating in bipolar mode and the clamped maximum available count would be exceeded
            * prior to incrementing the available count and waking of a thread.
            * The user provided function object is invoke prior to incrementing the available count.
            * This provides affords the client an opportunity to do bookkeeping under mutual exclusion without having to
            * take a separate lock.
            *
            * @param operation A reference to a user provided function object to be invoked prior during prior
            * to the available count being incremented. The user operation is invoked while an internal lock is held.
            * @warning Should the user provided operation throw an exception, the availableCount is not incremented and no thread
            * is awakened.
            * @throw Throws std::bad_function_call if the operation passed in has no target (an empty function object).
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation is invoked via another thread or if we have been
            * notified more times than we can count (2^32 -1).
            */
            void give( const FunctionType & operation );

            /**
            * @brief The Abort Operation
            *
            * This operation flags that the Semaphore has been aborted and wake any and all waiting threads.
            *
            * @note An aborted Semaphore is unrecoverable.
            */
            void abort();

            /**
            * @brief Get Available Count
            *
            * This is primarily an implementation validation operation. It returns a snapshot of the
            * current available count.
            *
            * @return Returns a snapshot of the available count at time of invocation.
            */
            size_t getAvailableCount();

        private:
            /**
            * @brief Pointer Member to Hidden Implementation
            *
            * This is our pointer to our hidden implementation.
            */
            Imple * pImple{ nullptr };
        };
    }
}


#endif /* REISERRT_CORE_SEMAPHORE_HPP */
