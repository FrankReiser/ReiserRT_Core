/**
* @file Semaphore.hpp
* @brief The Specification for Semaphore utility
* @authors: Frank Reiser
* @date Created on May 29, 2015
*/

#ifndef SEMAPHORE_HPP_
#define SEMAPHORE_HPP_

#include "ReiserRT_Core/ReiserRT_CoreExport.h"

#include <stdexcept>
#include <functional>

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief A Counted, Wait-able Semaphore Class
        *
        * This class provides a implementation of a thread-safe, wait-able counted semaphore similar to Unix System V
        * semaphores which are not part of the POSIX standard. Counted Semaphores are a resource management tool.
        * They are designed to efficiently manage resource availability, until resources are exhausted (count of zero)
        * at which point, a client must wait (block) until resources are released and made available for reuse.
        *
        * @todo Support move semantics.
        * @todo Support timed wait and test wait?
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
            * @brief The Function Type for Wait and Notify with Functor Interface
            *
            * This is the type of Functor expected for using wait or notify operations
            * taking a user provided functor to invoke while a lock is held. This should be a relatively simple operation
            * in order to keep lock duration period to a minimum. See wait and notify that accept functor arguments for additional
            * details.
            */
            using FunctionType = std::function< void() >;

            /**
            * @brief Qualified Constructor for Semaphore
            *
            * This operation constructs a Semaphore.
            *
            * @param theInitialCount The initial Semaphore count, defaults to zero and is clamped to
            * std::numeric_limits< uint32_t >::max() or slightly more than 4 billion (2^32 -1).
            */
            explicit Semaphore( size_t theInitialCount = 0 );

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
            * @brief The Wait Operation
            *
            * This operation attempts to decrement the available count towards zero.
            * If the available count is already zero, the operation will block until available count is incremented
            * away from zero.
            *
            * @throw Throws std::runtime_error if the abort operation is invoked via another thread.
            */
            void wait();

            /**
            * @brief The Wait Operation with Functor Interface
            *
            * This operation attempts to decrement the availableCount towards zero.
            * If the available count is already zero, the operation will block until availableCount is incremented
            * away from zero via a notify operation. When the wait has successfully taken an available count,
            * the user provided function object is invoked while an internal lock is held.
            * This provides affords the client an opportunity to do bookkeeping under mutual exclusion without having to
            * take a separate lock.
            *
            * @param operation This is a value (copy) of a user provided function object to be invoked after the availableCount is decremented.
            * The user operation is invoked while an internal lock is held. The function object may be wrapped with std::ref to avert overhead
            * in making a copy.
            * @warning Should the user operation throw an exception, the available count will be restored to its former state as if
            * the wait call was never invoked.
            * @warning It is expected that the function object remain valid throughout the duration of the wait invocation. Failure
            * to provide this assurance will lead to undefined behavior.
            * @throw Throws std::bad_function_call if the operation passed in has no target (an empty function object).
            * @throw Throws std::runtime_error if the abort operation is invoked via another thread.
            * @throw The user operation may throw an exception of unknown type.
            */
            void wait( FunctionType operation );

            /**
            * @brief The Notify Operation
            *
            * This operation increments the available count away from zero and will wake, at most, one waiting thread.
            *
            * @throw Throws std::runtime_error if the abort operation is invoked via another thread or if we have been
            * notified more times than we can count (2^32 -1).
            */
            void notify();

            /**
            * @brief The Notify Operation with Functor Interface
            *
            * This operation invokes a user provided function object while an internal lock is held.
            * It then increments the availableCount away from zero and will wake, at most, one waiting thread.
            * This provides affords the client an opportunity to do bookkeeping under mutual exclusion without having to
            * take a separate lock.
            *
            * @param operation This is a value (copy) of a user provided function object to be invoked prior during prior
            * to the available count being incremented. The user operation is invoked while an internal lock is held.
            * @warning Should the user provided operation throw an exception, the availableCount is not incremented and no thread
            * is awakened.
            * @warning It is expected that the function object remain valid throughout the duration of the wait invocation. Failure
            * to provide this assurance will lead to undefined behavior.
            * @throw Throws std::bad_function_call if the operation passed in has no target (an empty function object).
            * @throw Throws std::runtime_error if the abort operation is invoked via another thread or if we have been
            * notified more times than we can count (2^32 -1).
            */
            void notify( FunctionType operation );

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
            * @throw Throws std::runtime_error if the abort has been invoked.
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


#endif /* SEMAPHORE_HPP_ */
