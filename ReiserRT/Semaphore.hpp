/**
* @file Semaphore.hpp
* @brief The Specification for Semaphore utility
* @authors: Frank Reiser
* @date Created on May 29, 2015
*/

#ifndef SEMAPHORE_HPP_
#define SEMAPHORE_HPP_

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
        class Semaphore
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
            * std::numeric_limits< AvailableCountType>::max() or slightly more than 2 billion.
            */
            explicit Semaphore( size_t theInitialCount = 0 );

            /**
            * @brief Destructor for the Semaphore
            *
            * This destructor invokes the abort operation. If the Semaphore is still being
            * used by any thread when this destructor is invoked, those threads will be thrown an AbortedException.
            */
            ~Semaphore();

        private:
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

        public:
            /**
            * @brief The Wait Operation
            *
            * This operation attempts to decrement the availableCount towards zero.
            * If the availableCount is zero, the operation will block until availableCount is incremented
            * away from zero.
            *
            * @throw Throws std::runtime_error if the abort operation is invoked via another thread.
            */
            void wait();

            /**
            * @brief The Wait Operation with Functor Interface
            *
            * This operation attempts to decrement the availableCount towards zero.
            * If the availableCount is zero, the operation will block until availableCount is incremented
            * away from zero via a notify operation. Once the availableCount has successfully been incremented,
            * the user provided function object is invoked while an internal lock is held.
            *
            * @param operation This is a value (copy) of a user provided function object to be invoked after the availableCount is decremented.
            * The user operation is invoked while an internal lock is held.
            * @warning Should the user operation throw an exception, an availableCount may be wasted, as it has already been decremented.
            * It is the responsibility of the implementor of such an operation to decide if this is recoverable and a notify call can be utilized
            * to restore the availableCount.
            * @todo We can do better than this. We already have in several places using RAII techniques. Actually, it's more complicated
            * than that. You don't want the throw an exception. I am not preventing it because it's certainly possible in my own
            * code. However, I feel that it is a design problem on the outside. If the design "managed" things it shouldn't happen.
            * These hooks are intended to be used for tight code in a aggregating class. I.e., bookkeeping chores. Document better.
            * @warning It is expected that the function object remain valid throughout the duration of the wait invocation. Failure
            * to provide this assurance will lead to undefined behavior.
            * @throw Throws std::bad_function_call if the operation passed in has no target (an empty function object).
            * @throw Throws std::runtime_error if the abort operation is invoked via another thread.
            */
            void wait( FunctionType operation );

            /**
            * @brief The Notify Operation
            *
            * This operation increments the availableCount away from zero and will wake, at most, one waiting thread.
            *
            * @throw Throws std::runtime_error if the abort operation is invoked via another thread.
            */
            void notify();

            /**
            * @brief The Notify Operation with Functor Interface
            *
            * This operation invokes a user provided function object while an internal lock is held.
            * It then increments the availableCount away from zero and will wake, at most, one waiting thread.
            *
            * @param operation This is a value (copy) of a user provided function object to be invoked prior during prior to the availableCount
            * being incremented. The user operation is invoked while an internal lock is held.
            * @warning Should the user provided operation throw an exception, the availableCount is not incremented.
            * It is the responsibility of the implementor of such an operation to take appropriate measures to prevent resource leakage.
            * @warning It is expected that the function object remain valid throughout the duration of the wait invocation. Failure
            * to provide this assurance will lead to undefined behavior.
            * @throw Throws std::bad_function_call if the operation passed in has no target (an empty function object).
            * @throw Throws std::runtime_error if the abort operation is invoked via another thread.
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
            * @return Returns a snapshot of the availableCount at time of invocation.
            */
            size_t getAvailableCount();

        private:
            /**
            * @brief Pointer Member to Hidden Implementation
            *
            * This is our pointer to our hidden implementation.
            * @todo Make this a unique_ptr and make the class 'movable'.
            */
            Imple * pImple;
        };
    }
}


#endif /* SEMAPHORE_HPP_ */
