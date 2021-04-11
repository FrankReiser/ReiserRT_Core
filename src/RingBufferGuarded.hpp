/**
* @file RingBufferGuardedBase.hpp
* @brief The Specification file for RingBufferGuardedBase
* @authors Frank Reiser
* @date Created on Apr 10, 2017
*/

#ifndef REISERRT_CORE_RINGBUFFERGUARDED_HPP
#define REISERRT_CORE_RINGBUFFERGUARDED_HPP

#include "RingBufferSimple.hpp"
#include "Semaphore.hpp"

#include <atomic>

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief RingBufferGuardedBase Class
        *
        * This template class provides a base for more specialized types.
        * It employs a counted semaphore object which block get operations on a low or empty ring buffer.
        * @note The is no blocking on a full condition. Doing so was not a requirement, nor design goal
        * for ReiserRT::Core. Providing such a capability at this level would impact performance and if needed
        * can be layered on top.
        *
        * @tparam T The ring buffer element type.
        * @note Must be a scalar type (e.g., char, int, float or void pointer).
        */
        template< typename T >
        class RingBufferGuardedBase : public RingBufferSimple< T >
        {
        private:
            // T must be a valid scalar type for rapid load and store operations. Non-scalar types are supportable
            // through a type pointer. See class ObjectPool within this namespace for such a use case.
            static_assert( std::is_scalar< T >::value,
                    "RingBufferGuardedBase< T > must specify a scalar type (which includes pointer types)!" );

            /**
            * @brief Alias Type to RingBufferBase< T >
            *
            * This type provides a little "syntactic sugar" for the class.
            */
            using Base = RingBufferSimple< T >;

            /**
            * @brief Semaphore Type
            *
            * This type is used to used to wait when nothing left is available to get.
            */
            using SemaphoreType = ReiserRT::Core::Semaphore;

            /**
            * @brief The Ring Buffer State
            *
            * Our guarded ring buffer provides numerous operation that should only be invoked when in the appropriate
            * state. For instance, it would be inappropriate to call get when the ring buffer is in the "Terminal" state.
            * There are other preconditions which will be discussed within the various operation documentation.
            */
            enum class State : uint8_t
            {
                NeedsPriming=0,     //!< The ring buffer was constructed with a non-zero willPrime argument value.
                Ready,              //!< The ring buffer is ready for regular operational usage (e.g., get and put)
                Terminal,           //!< The ring buffer is in the "Terminal" state. This is a final state.
            };

            /**
            * @brief Atomic State Type
            *
            * The state is managed atomically, outside the context of our semaphore locks.
            */
            using AtomicStateType = std::atomic< State >;

        public:
            /**
            * @brief Qualified Constructor for RingBufferGuardedBase
            *
            * This constructor first delegates to its Base class to build a "simple" ring buffer of requestedNumElements.
            * It then instantiates its counted semaphore with an initialCount. For an initially empty ring buffer,
            * the willPrime argument should be false. For an initially populated ring buffer, willPrime
            * should be non-zero (or true). In this case, the prime operation must be called to initially populated the requested
            * number of elements to make the instance sane.
            *
            * @param requestedNumElements The requested number of elements. This is always rounded up to the next power of two for
            * the ring buffer itself, but not for the semaphore available count.
            * @param willPrime If non-zero,the ring buffer must be subsequently primed by invoking the prime operation.
            * @warning Failure to prime the instance will result in exceptions being thrown via get and put operations.
            */
            explicit RingBufferGuardedBase( size_t theRequestedNumElements, bool willPrime = false )
                : Base{ theRequestedNumElements }
                , semaphore{ willPrime ? theRequestedNumElements : 0 }
                , state{ willPrime ? State::NeedsPriming : State::Ready }
            {
            }

            /**
            * @brief Destructor for the RingBufferGuardedBase
            *
            * This destructor invokes the abort operation on its semaphore instance prior to base object destruction.
            * This would be a do nothing if the semaphore was previously aborted; which would be highly appropriate to do,
            * allowing other threads to get out of the way of the impending destruction. Therefore, this is a last ditch
            * attempt to ensure we are aborted.
            */
            ~RingBufferGuardedBase() { abort(); }

            /**
            * @brief The Abort Operation
            *
            * This operation aborts the counted semaphore which will wake all pending get threads.  Such threads
            * will experience a Semaphore::AbortedException.
            */
            inline void abort() { state = State::Terminal; semaphore.abort(); }

            /**
            * @brief The Get Operation
            *
            * This operation will retrieve a value from the base implementation. If the base is in an empty condition, it
            * will block until a value becomes available. The base class "get" functionality is invoked while in the context
            * of our counted semaphore's internal lock. This is why we can employ the "simple" ring buffer as our base, which
            * is a very lean implementation.
            *
            * Of interest here is how we wrap the actual get operation within a "lambda" and pass a reference wrapper to this lambda
            * into the semaphore wait operation. This essentially allows it to loop back into our stack frame while in the context
            * of its lock to invoke the get operation and set our return value. The wait operation has no idea of what is actually
            * being accomplished. The use of the reference wrapper ensures the temporary Semaphore::FunctorType does not need to
            * access the heap for its internals.
            *
            * @pre The ring buffer is expected to be in the "Ready" state to invoke this operation. Violations will result in an exception
            * being thrown.
            *
            * @throw Throws ReiserRT::Core::RingBufferStateError if not in the "Ready" state.
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the semaphore is aborted.
            *
            * @return Returns a value of T retrieved from the implementation.
            */
            inline T get()
            {
                // We have to be in the "Ready" state, or we will throw a logic error.
                if ( state != State::Ready )
                {
                    throw RingBufferStateError{ "RingBufferGuarded::get invoked while not in the Ready state!" };
                }

                // Instantiate return variable and setup a lambda to be invoked in the context of the semaphore's internal lock.
                // We are 100% confident that this will not throw an underflow exception because the ring buffer available elements to "get"
                // are at least that of the semaphore's available count and it will block if this is not the case.
                T retVal;
                auto getFunk = [ this, &retVal ]() { retVal = this->Base::get(); };

                // Invoke semaphore wait passing our lambda and return the result.
                semaphore.wait( std::ref( getFunk ) );
                return retVal;
            }

            /**
            * @brief The Put Operation
            *
            * This operation will put a value into the base implementation. The base class "put" functionality is invoked while in the context
            * of our counted semaphore's internal lock. This is why we can employ the "simple" ring buffer as our base, which
            * is a very lean implementation.
            *
            * Of interest here is how we wrap the actual put operation within a "lambda" and pass a reference wrapper to this lambda
            * into the semaphore notify operation. This essentially allows it to loop back into our stack frame while in the context
            * of its lock to invoke the put operation and load our value. The notify operation has no idea of what is actually
            * being accomplished. The use of the reference wrapper ensures the temporary Semaphore::FunctorType does not need to
            * access the heap for its internals.
            *
            * @pre The ring buffer is expected to be in the "Ready" state to invoke this operation. Violations will result in an exception
            * being thrown.
            *
            * @throw Throws ReiserRT::Core::RingBufferStateError if not in the "Ready" state.
            * @throw Throws ReiserRT::Core::RingBufferOverflow if our base class is in a "Full" state. No value is made available for subsequent retrieval.
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the semaphore is aborted.
            *
            * @param p val A value to be put into the ring buffer implementation.
            */
            inline void put( T val )
            {
                // If we are in the terminal state, we will just get out of the way
                if ( state == State::Terminal ) return;

                // We have to be in the "Ready" state, or we will throw a logic error.
                if ( state != State::Ready )
                {
                    throw RingBufferStateError{ "RingBufferGuarded::put invoked while not in the Ready state!" };
                }

                // Setup a lambda to be invoked in the context of the semaphore's internal lock.
                // There is no guarding of overflow here. If it throws, the RingBuffer is not being serviced adequately.
                // It is up to the client to manage and/or mitigate this possibility.
                auto putFunk = [ this, val ]() { this->Base::put( val ); };
                semaphore.notify( std::ref( putFunk ) );
            }

            /**
            * @brief The Priming Function Type
            *
            * This function object type (a "functor") provides specification for a callable object that
            * returns a type "T" (our template parameter), and has one argument which represents an index or
            * counter. A value of this type is required for the prime operation. This operation is not expected to throw
            * an exception.
            */
            using PrimingFunctionType = std::function< T( size_t ) noexcept >;

            /**
            * @brief The Prime Operation
            *
            * This operation will call the user provided function object, in a loop, until all the initially
            * requested number of elements are all enqueued into the ring buffer.
            *
            * @param operation This is a functor value of type PrimingFunctionType. It must have a valid target (non-empty function object).
            *
            * @pre The ring buffer is expected to be in the "Ready" state to invoke this operation. Violations will result in an exception
            * being thrown.
            *
            * @throw Throws ReiserRT::Core::RingBufferStateError if the RingBufferGuardedBase is not in the "NeedsPriming" state.
            * @throw Throws std::bad_function_call if the operation passed in has no target (i.e., an empty function object).
            */
            void prime( PrimingFunctionType operation )
            {
                // We have to be in the "NeedsPriming" state, or we will throw a logic error.
                if ( state != State::NeedsPriming )
                {
                    throw RingBufferStateError{ "RingBufferGuarded::prime invoked while not in the NeedsPriming state!" };
                }

                // We allow for priming only the original number of requested elements. Not the ring buffer size,
                // which is always rounded up to the next power of two. We also check to see if an abort has been detected
                // which may terminate the loop prematurely. The state should remain NeedsPriming until we set it otherwise.
                size_t count = semaphore.getAvailableCount();
                for ( size_t i = 0; i != count && state == State::NeedsPriming; ++i )
                {
                    Base::put( operation( i ) );
                }

                // Here, we ensure that we can set the state to Ready unless an abort is detected.
                State currentState = state;
                do
                {   // If the current state transitions to Terminal, then we will not go "Ready".
                    if ( currentState == State::Terminal ) return;
                } while ( !state.compare_exchange_weak( currentState, State::Ready ) );
            }

            /**
            * @brief The Flushing Function Type
            *
            * This function object type (a "functor") provides specification for a callable object that
            * returns void and accepts on argument value of type T (our template parameter).
            * A value of this type is required for the flush operation. This operation is not expected to throw
            * an exception.
            */
            using FlushingFunctionType = std::function< void( T ) noexcept >;

            /**
            * @brief The Flush Operation
            *
            * This operation will call the user provided function object, in a loop, until the ring buffer is emptied.
            * It is provided for use cases where a client puts pointers to objects a ring buffer, which should be
            * properly destroyed when the ring buffer instance is going to be destroyed. It need not be utilized for
            * ring buffers of simple intrinsic scalar types, such as char or int.
            *
            * @param operation This is a functor value of type FlushingFunctionType. It must have a valid target (i.e., an empty function object).
            *
            * @pre The ring buffer is expected to be in the "Terminal" state to invoke this operation. Violations will result in an exception
            * being thrown.
            *
            * @throw Throws ReiserRT::Core::RingBufferStateError if the RingBufferGuardedBase is not in the "Terminal" state.
            * @throw Throws std::bad_function_call if the operation passed in has no target (an empty function object).
            */
            void flush( FlushingFunctionType operation )
            {
                // We have to be in the "Terminal" state, or we will throw a logic error.
                if ( state != State::Terminal )
                {
                    throw RingBufferStateError{ "RingBufferGuarded::flush invoked while not in the Terminal state!" };
                }

                const size_t count = semaphore.getAvailableCount();
                for ( size_t i = 0; count != i; ++i ) operation( Base::get() );
            }

            /**
            * @brief Inherit the Get Number of Bits Operation
            *
            * This declaration brings the getNumBits operation from our Base class into the public scope.
            */
            using Base::getNumBits;

            /**
            * @brief Inherit the Get Size Operation
            *
            * This declaration brings the getSize operation from our Base class into the public scope.
            */
            using Base::getSize;

            /**
            * @brief Inherit the Get Mask Operation
            *
            * This declaration brings the getMask operation from our Base class into the public scope.
            */
            using Base::getMask;

        private:
            /**
            * @brief The Counted Semaphore Object.
            *
            * This object encapsulates and fulfills all the necessary requirements to make this object a "pendable"
            * thread safe, ring buffer.
            */
            SemaphoreType semaphore;

            /**
            * @brief Our Atomic State Variable
            *
            * This attribute tracks our state, using C++11 atomic data types which is lock-free, thread safe.
            */
            AtomicStateType state;
        };

        /**
        * @brief RingBufferGuarded Class
        *
        * This template class provides a template for simple scalar types (not pointer types). It is derived from
        * RingBufferGuardedBase of the same template argument type which own an implementation instance.
        *
        * @tparam T The ring buffer element type (not for pointer types).
        * @note Must be a scalar type (e.g., char, int, float).
        */
        template< typename T >
        class RingBufferGuarded : public RingBufferGuardedBase< T >
        {
        private:
            /**
            * @brief Alias Type to RingBufferGuardedBase< T >
            *
            * This type provides a little "syntactic sugar" for the class.
            */
            using Base = RingBufferGuardedBase< T >;

        public:
            /**
            * @brief Inherit Constructors from Base Class
            *
            * We add no additional attributes. This declaration specifies that we
            * want the Base version of constructors exposed as our own.
            */
            using Base::RingBufferGuardedBase;

            /**
            * @brief Inherit the Get Operation
            *
            * This declaration brings the get operation from our Base class into the public scope.
            */
            using Base::get;

            /**
            * @brief Inherit the Put Operation
            *
            * This declaration brings the put operation from our Base class into the public scope.
            */
            using Base::put;

            /**
            * @brief Inherit the Abort Operation
            *
            * This declaration brings the abort operation from our Base class into the public scope.
            */
            using Base::abort;

            /**
            * @brief Inherit the Prime Operation
            *
            * This declaration brings the prime operation from our Base class into the public scope.
            */
            using Base::prime;

            /**
            * @brief Inherit the Flush Operation
            *
            * This declaration brings the flush operation from our Base class into the public scope.
            */
            using Base::flush;

            /**
            * @brief Inherit the Get Number of Bits Operation
            *
            * This declaration brings the getNumBits operation from our Base class into the public scope.
            */
            using Base::getNumBits;

            /**
            * @brief Inherit the Get Size Operation
            *
            * This declaration brings the getSize operation from our Base class into the public scope.
            */
            using Base::getSize;

            /**
            * @brief Inherit the Get Mask Operation
            *
            * This declaration brings the getMask operation from our Base class into the public scope.
            */
            using Base::getMask;
        };

        /**
        * @brief Specialization of RingBufferGuarded for void Pointer Type
        *
        * This specialization of the RingBufferGuardedBase is specifically designed to handle void pointer
        * type values. It was designed to handle a wide variety of concrete typed pointers
        * that can transparently be cast back and forth with void pointers, thereby simplifying
        * the type pointer template. It inherits directly from the RingBufferGuardedBase.
        */
        template<>
        class RingBufferGuarded< void * > : public RingBufferGuardedBase< void * >
        {
        private:
            /**
            * @brief Alias Type to RingBufferGuardedBase< void * >
            *
            * This type provides a little "syntactic sugar" for the class.
            */
            using Base = RingBufferGuardedBase< void * >;

        public:
            /**
            * @brief Inherit Constructors from Base Class
            *
            * We add no additional attributes. This declaration specifies that we
            * want the Base version of constructors exposed as our own.
            */
            using Base::RingBufferGuardedBase;

            /**
            * @brief Inherit the Get Operation
            *
            * This declaration brings the get operation from our Base class into the public scope.
            */
            using Base::get;

            /**
            * @brief Inherit the Put Operation
            *
            * This declaration brings the put operation from our Base class into the public scope.
            */
            using Base::put;

            /**
            * @brief Inherit the Abort Operation
            *
            * This declaration brings the abort operation from our Base class into the public scope.
            */
            using Base::abort;

            /**
            * @brief Inherit the Prime Operation
            *
            * This declaration brings the prime operation from our Base class into the public scope.
            */
            using Base::prime;

            /**
            * @brief Inherit the Flush Operation
            *
            * This declaration brings the flush operation from our Base class into the public scope.
            */
            using Base::flush;

            /**
            * @brief Inherit the Get Number of Bits Operation
            *
            * This declaration brings the getNumBits operation from our Base class into the public scope.
            */
            using Base::getNumBits;

            /**
            * @brief Inherit the Get Size Operation
            *
            * This declaration brings the getSize operation from our Base class into the public scope.
            */
            using Base::getSize;

            /**
            * @brief Inherit the Get Mask Operation
            *
            * This declaration brings the getMask operation from our Base class into the public scope.
            */
            using Base::getMask;
        };

        /**
        * @brief A Partial Specialization for Concrete Type Pointers
        *
        * This partial specialization is provided for pointers to any type to be put into or retrieved from
        * a RingBufferGuarded. It relies on its base class, RingBufferGuarded< void * >, for all but a cast to and fro.
        */
        template< typename T >
        class RingBufferGuarded< T * > : public RingBufferGuarded< void * >
        {
        private:
            /**
            * @brief Alias Type to RingBufferGuarded< void * >
            *
            * This type provides a little "syntactic sugar" for the class.
            */
            using Base = RingBufferGuarded< void * >;

            /**
            * @brief Required Put Type
            *
            * Since the RingBufferImple cannot "put" into a constant buffer location because its internal representation is
            * not constant.  This causes problems when type T is constant.  Any constant qualifier must be removed.
            * However, rest assured. The implementation will not mute any such data. Doing so would make it pretty much
            * useless.
            */
            using PutType = typename std::remove_const<T>::type *;

        public:
            /**
            * @brief Inherit Constructors from Base Class
            *
            * We add no additional attributes. This declaration specifies that we
            * want the Base version of constructors exposed as our own.
            */
            using Base::RingBufferGuarded;

            /**
            * @brief Inherit the Abort Operation
            *
            * This declaration is to satisfy the Eclipse IDE syntax analyzer and nothing else.
            */
            using Base::abort;

            /**
            * @brief Inherit the Prime Operation
            *
            * This declaration is to satisfy the Eclipse IDE syntax analyzer and nothing else.
            */
            using Base::prime;

            /**
            * @brief Inherit the Flush Operation
            *
            * This declaration is to satisfy the Eclipse IDE syntax analyzer and nothing else.
            */
            using Base::flush;

            /**
            * @brief The Get Operation
            *
            * This operation invokes the base class to retrieve a void pointer to the specified type
            * from the base RingBufferGuarded.
            * The value is compile time converted to a pointer of the specified templated type. Thereby,
            * adding no run-time penalty.
            *
            * @throw Throws ReiserRT::Core::RingBufferStateError if not in the "Ready" state.
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the semaphore is aborted.
            *
            * @return Returns a pointer to an object of type T retrieved from the implementation.
            */
            inline T * get() { return reinterpret_cast< T* >( Base::get() ); }

            /**
            * @brief The Put Operation
            *
            * This operation invokes the base class to put a typed pointer value into the base RingBufferGuarded.
            * Any constant specification is removed and the typed pointer is implicitly converted to a
            * void pointer at compile time yielding no run-time penalty.
            *
            * @throw Throws ReiserRT::Core::RingBufferStateError if not in the "Ready" state.
            * @throw Throws ReiserRT::Core::RingBufferOverflow if there is no room left to fulfill the request (full).
            *
            * @param p A pointer to the object to be put into the ring buffer implementation.
            */
            inline void put( T * p ) { Base::put( const_cast< PutType >( p ) ); }
        };

    }
}

#endif /* REISERRT_CORE_RINGBUFFERGUARDED_HPP */

