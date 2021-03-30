/**
* @file ObjectQueue.hpp
* @brief The Specification for a Pendable ObjectQueue
* @authors Frank Reiser
* @date Created on Apr 4, 2015
*/

#ifndef REISERRT_CORE_OBJECTQUEUE_HPP
#define REISERRT_CORE_OBJECTQUEUE_HPP

#include "ReiserRT_CoreExport.h"

#include <cstdint>
#include <type_traits>
#include <functional>

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief The ObjectQueueBase Class
        *
        * This class provides a base for all specialized ObjectQueue template instantiations.
        * It provides a hidden implementation and the primary interface operations required for ObjectQueue.
        */
        class ReiserRT_Core_EXPORT ObjectQueueBase
        {
        private:
            /**
            * @brief Forward Declaration of Imple
            *
            * We forward declare our Hidden Implementation.
            */
            class Imple;

        public:
            /**
            * @brief Performance Tracking Feature Counter Type
            *
            * The ObjectQueue can keep track of certain performance characteristics. These would primarily be a "High Watermark"
            * and the current "Running Count". Since this is a queue, the "Running Count" starts out empty (0) and so would
            * the "High Watermark". We use a 32 bit value for these, which should be adequate.
            */
            using CounterType = uint32_t;

            /**
            * @brief A Return Type for Inquiring Clients
            *
            * This structure provides the essential information for clients inquiring on performance
            * measurements. It represents a snapshot of the current state of affairs at invocation.
            * The "High Watermark" is significantly more stable than the "Running Count" which may vary irradicatly,
            * dependent on the client.
            */
            struct RunningStateStats
            {
                /**
                * @brief Default Constructor for RunningStateStats
                *
                * This operation uses the compiler generated default, which in our case is to default the data members to zero.
                */
                RunningStateStats() noexcept = default;

                size_t size{ 0 };               //!< The Size Requested at Construction.
                CounterType runningCount{ 0 };  //!< The Current Running Count Captured Atomically (snapshot)
                CounterType highWatermark{ 0 }; //!< The Current High Watermark Captured Atomically (snapshot)
            };

            /**
            * @brief Flushing Function Type
            *
            * The flushing function is used to destroy any objects left in the queue. The implementation of such
            * function would need to cast to the correct type inorder to invoke the required destructor.
            * This function is not excepted to throw an exception.
            */
            using FlushingFunctionType = std::function< void( void * ) noexcept >;

            /**
            * @brief Default Constructor for ObjectQueueBase
            *
            * Default construction of ObjectQueue is disallowed. Hence, this operation has been deleted.
            */
            ObjectQueueBase() = delete;

            /**
            * @brief Copy Constructor for ObjectQueueBase
            *
            * Copying ObjectQueueBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a ObjectQueueBase.
            */
            ObjectQueueBase( const ObjectQueueBase & another ) = delete;

            /**
            * @brief Copy Assignment Operation for ObjectQueueBase
            *
            * Copying ObjectQueueBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a ObjectQueueBase.
            */
            ObjectQueueBase & operator =( const ObjectQueueBase & another ) = delete;

            /**
            * @brief Move Constructor for ObjectQueueBase
            *
            * Moving ObjectQueueBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a ObjectQueueBase.
            */
            ObjectQueueBase( ObjectQueueBase && another ) = delete;

            /**
            * @brief Move Assignment Operation for ObjectQueueBase
            *
            * Moving ObjectQueueBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a ObjectQueueBase.
            */
            ObjectQueueBase & operator =( ObjectQueueBase && another ) = delete;

        protected:
            /**
            * @brief Qualified ObjectQueueBase Constructor
            *
            * This constructor build the ObjectQueue of exactly the requested size. Its internal
            * RingBuffer objects will be up-sized to the next power of two. It delegates to its hidden
            * implementation to do the heavy lifting.
            *
            * @param requestedNumElements The number of elements requested for the ObjectQueue.
            * @param elementSize The size of each element.
            *
            * @throw Throws std::bad_alloc if memory requirements for the Implementation cannot be satisfied.
            *
            * @note Requesting somewhat less than a whole power of two, for requestedNumElements provides a bit of headroom within
            * the implementation. However, doing so is not necessary.
            */
            ObjectQueueBase( size_t requestedNumElements, size_t elementSize );

            /**
            * @brief The ObjectQueueBase Destructor
            *
            * This operation deletes its implementation instance.
            */
            ~ObjectQueueBase();

            /**
            * @brief The abort Operation
            *
            * This operation is invoked to abort the ObjectQueueBase. It delegates to its implementation
            * to perform the operation.
            *
            * @note Once aborted, there is no recovery. Further put, emplace or get operations will receive a std::runtime_error.
            */
            void abort();

            /**
            * @brief The rawWaitAndGet Operation
            *
            * This operation is invoked to obtain raw memory from the raw implementation.
            *
            * @throw Throws std::runtime_error if the "raw" ring buffer becomes aborted.
            *
            * @return Returns a void pointer to raw memory from the raw ring buffer.
            */
            void * rawWaitAndGet();

            /**
            * @brief The cookedPutAndNotify Operation
            *
            * This operation loads cooked memory into the implementation.
            *
            * @param pCooked A pointer to an object of unknown type.
            *
            * @throw Throws std::runtime_error if the "cooked" ring buffer becomes aborted.
            */
            void cookedPutAndNotify( void * pCooked );

            /**
            * @brief The cookedWaitAndGet Operation
            *
            * This operation is invoked to obtain cooked memory from the implementation..
            *
            * @throw Throws std::runtime_error if the "cooked" ring buffer becomes aborted.
            *
            * @return Returns memory for an object of unknown type from the cooked ring buffer.
            */
            void * cookedWaitAndGet();

            /**
            * @brief The rawPutAndNotify Operation
            *
            * This operation loads raw memory into the implementation.
            *
            * @param pRaw A pointer to memory that is available for cooking.
            *
            * @throw Throws std::runtime_error if the "raw" ring buffer becomes aborted.
            */
            void rawPutAndNotify( void * pRaw );

            /**
            * @brief The Flush Operation
            *
            * This operation is provided to empty the "cooked" ring buffer and properly destroy any unfetched objects that may remain.
            * Being that we only deal with void pointers at this level, what do not know what type of destructor to invoke.
            * However, through a user provided function object, the destruction can be elevated to the client level, which should know
            * what type of base objects may remain.
            *
            * @pre The implementation's "cooked" ring buffer is expected to be in the "Terminal" state to invoke this operation.
            * Violations will result in an exception being thrown.
            *
            * @param operation This is a functor value of type FlushingFunctionType. It must have a valid target
            * (i.e., an empty function object). It will be invoked once per remaining object in the "cooked" ring buffer.
            *
            * @throw Throws std::bad_function_call if the operation passed in has no target (an empty function object).
            */
            void flush( FlushingFunctionType operation );

            /**
            * @brief The Get Running State Statistics Operation
            *
            * This operation provides for performance monitoring of the ObjectQueue.
            *
            * @return Returns a snapshot of RunningStateStats.
            */
            RunningStateStats getRunningStateStatistics() noexcept;

            /**
            * @brief The Is Aborted Operation
            *
            * This operation provides a means to detect whether the abort operation has been preformed.
            * @return
            */
            bool isAborted() noexcept;

            /**
            * @brief The Hidden Implementation Instance
            *
            * This attribute stores an instance of our hidden implementation.
            */
            Imple * pImple;
        };

        /**
        * @brief A Generic, Wait-able, Object Queue
        *
        * This template class provides a generic implementation of a thread-safe, wait-able object queue.
        * Putting objects into a this queue should not require any significant waiting
        * as a well serviced queue should be relatively empty. On the other hand, getting objects
        * out of a well serviced queue typically involves waiting for object to be put into the queue.
        *
        * This implementation makes extensive use of RingBuffer and Semaphore of the same namespace.
        *
        * @tparam T The type of object that will be put into and received from.
        * @note ObjectQueue does not currently support enqueueing and de-queueing of derived types of T.
        * It is of primary utility to MessageQueue which pushes a unique_ptr type through it.
        *
        * @warning Type T must be no-throw, move constructible and no-throw destructible.
        */
        template < typename T >
        class ObjectQueue : public ObjectQueueBase
        {
            static_assert( std::is_move_constructible<T>::value, "Type T must be move constructible!!!" );
            static_assert( std::is_nothrow_destructible<T>::value, "Type T must be no throw destructible!!!" );

        private:
            /**
            * @brief The Type Size
            *
            * This compile-time constant records the size in bytes of our template parameter type T.
            */
            constexpr static size_t typeSize = sizeof( T );

            /**
            * @brief Allocation Size Alignment Overspill
            *
            * This compile-time constant is essentially the remainder of typeSize
            * divided by the size of the architecture. It is use to determine a padding to bring this size to the next
            * multiple of architecture size.
            */
            constexpr static size_t alignmentOverspill = typeSize % sizeof( void * );

            /**
            * @brief The Padded Allocation Size
            *
            * This compile-time constant value is what we will allocate to each object put (moved) into the queue.
            * This ensures that all enqueued objects, ultimately moved into the arena, are architecture aligned.
            */
            constexpr static size_t paddedTypeAllocSize = ( alignmentOverspill != 0 ) ? typeSize + sizeof( void * ) - alignmentOverspill : typeSize;

        public:
            /**
            * @brief A Reserved Put Handle
            *
            * This class serves as a "handle" to reserve a put into the Queue. This is not an actual
            * slot location, but a take of (wait on) the rawSemaphore. With this handle, your are guaranteed
            * that a putOnReserved, or an emplaceOnReserved operation will succeed without having to wait.
            * No extra priority (slot location) is inferred. Other objects can still be enqueued/dequeue before you
            * use this handle. Once utilized in putOnReserved or emplaceOnReserved, it may not be used again.
            * Subsequent reuse will be silently ignored.
            *
            * @note If this object goes out of scope and is destroyed before it is used, the reservation is silently aborted.
            */
            class ReservedPutHandle
            {
            private:
                /**
                * @brief Friend Declaration
                *
                * Our specialized parent ObjectQueue is a friend and only it can invoke our "Qualified Constructor" which specifies
                * an instance of the specialization.
                */
                friend class ObjectQueue;

                /**
                * @brief Qualified Constructor for ReservedPutHandle
                *
                * Constructs a ReservedPutHandle instance. We record the Queue instance and the pointer to raw memory
                * to subsequently move data type T into.
                *
                * @param theQueue A pointer to the ObjectQueue instance
                * @param theRaw A pointer to the raw memory to move subsequently move data type T into.
                */
                ReservedPutHandle( ObjectQueue< T > * theQueue, void * theRaw ) noexcept : pQueue{ theQueue }, pRaw( theRaw ) {}

            public:
                /**
                * @brief Default Constructor for ReservedPutHandle
                *
                * Default construction is disallowed for this class. Hence, this operation has been deleted.
                */
                ReservedPutHandle() = delete;

                /**
                * @brief Copy Constructor for ReservedPutHandle
                *
                * Copying of ReservedPutHandle is is disallowed for this class. Hence, this operation has been deleted.
                *
                * @param another A reference to an object instance being copied.
                */
                ReservedPutHandle( const ReservedPutHandle & another ) = delete;

                /**
                * @brief Copy Assignment Operator for ReservedPutHandle
                *
                * Copying of ReservedPutHandle is is disallowed for this class. Hence, this operation has been deleted.
                *
                * @param another A reference to an object instance being copied.
                */
                ReservedPutHandle & operator = ( const ReservedPutHandle & another ) = delete;

                /**
                * @brief Move Constructor for ReservedPutHandle
                *
                * This operation move constructs a ReservedPutHandle from another instance.
                * Instance another is neutered in the process.
                *
                * @param another A reference to an object instance being moved from.
                */
                ReservedPutHandle( ReservedPutHandle && another ) noexcept : pQueue{ another.pQueue }, pRaw{ another.pRaw }
                {
                    another.pRaw = nullptr;
                }

                /**
                * @brief Move Assignment Operator for ReservedPutHandle
                *
                * This operation move assigns a ReservedPutHandle from another instance.
                * Instance another is neutered in the process.
                *
                * @param another A reference to an object instance being moved from.
                */
                ReservedPutHandle & operator= ( ReservedPutHandle && another ) noexcept
                {
                    pQueue = another.pQueue;
                    pRaw = another.pRaw;
                    another.pRaw = nullptr;

                    return *this;
                }

                /**
                * @brief Destructor for ReservedPutHandle
                *
                * This destructor aborts the "reserved put" returning the raw memory to the pool.
                * It first checks to make sure that pRaw is still valid prior the aborting it.
                */
                ~ReservedPutHandle() { if ( pRaw ) { pQueue->abortReservedPut( pRaw ); pRaw = nullptr; } }

            private:
                /**
                * @brief The ObjectQueue Instance That Instantiated This
                *
                * This is a pointer reference to the ObjectQueue instance that instantiated this instance.
                * It is used in our destructor to abort the reservation and return the raw memory back to the pool.
                */
                ObjectQueue * pQueue;

                /**
                * @brief Pointer to Raw Memory
                *
                * This is the pointer to raw memory from the arena where an enqueue object will be moved to.
                */
                void * pRaw;
            };

            /**
            * @brief Default Constructor for ObjectQueue
            *
            * Default construction of ObjectQueue is disallowed. Hence, this operation has been deleted.
            */
            ObjectQueue() = delete;

            /**
            * @brief Qualified ObjectQueue Constructor
            *
            * This constructor build the ObjectQueue of exactly the requested size. Its internal
            * RingBuffer objects will be up-sized to the next power of two. It delegates to its base
            * class ObjectQueueBase to do the heavy lifting.
            *
            * @param requestedNumElements The number of elements requested for the ObjectQueue.
            *
            * @throw Throws std::bad_alloc if memory requirements for the Implementation cannot be satisfied.
            *
            * @note Requesting somewhat less than a whole power of two, for requestedNumElements provides a bit of headroom within
            * the implementation. However, doing so is not necessary.
            */
            explicit ObjectQueue( size_t requestedNumElements );

            /**
            * @brief Destructor for ObjectQueue
            *
            * This is our destructor for the ObjectQueue. It first invokes the abort operation of its
            * base class and then destroys any objects that may be left in the cooked queue.
            */
            ~ObjectQueue();

            /**
            * @brief Copy Constructor for ObjectQueue
            *
            * Copying ObjectQueue is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a ObjectQueue of the same templated type.
            */
            ObjectQueue( const ObjectQueue & another ) = delete;

            /**
            * @brief Copy Assignment Operation for ObjectQueue
            *
            * Copying ObjectQueue is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a ObjectQueue of the same templated type.
            */
            ObjectQueue & operator =( const ObjectQueue & another ) = delete;

            /**
            * @brief Move Constructor for ObjectQueue
            *
            * Moving ObjectQueue is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a ObjectQueue of the same templated type.
            */
            ObjectQueue( ObjectQueue && another ) = delete;

            /**
            * @brief Move Assignment Operation for ObjectQueue
            *
            * Moving ObjectQueue is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a ObjectQueue of the same templated type.
            */
            ObjectQueue & operator =( ObjectQueue && another ) = delete;

            /**
            * @brief The Put Operation
            *
            * The put operation enqueues an object into the ObjectQueue.
            *
            * @note This operation will block if the ObjectQueue is full.
            *
            * @param obj A reference to the object to be enqueued.
            *
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked.
            */
            void put( T & obj );

            /**
            * @brief The Emplace Operation
            *
            * The emplace operation employs a C++11 variadic template specification. It works almost exactly like
            * the put operation. The difference is that the arguments are
            * "perfect forwarded" and constructed in-place on raw memory, effectively saving a move operation
            * at a minimum.
            *
            * @note This operation will block if the ObjectQueue is full.
            *
            * @param args Whatever arguments satisfy a particular constructor overload for type T, possibly including
            * zero arguments.
            *
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked.
            */
            template < typename... Args >
            void emplace( Args&&... args );

            /**
            * @brief The Get Operation
            *
            * The get operation dequeues an object from the ObjectQueue by moving it out into client storage.
            *
            * @note This operation will block if the ObjectQueue is empty which is the general expectation
            * of a well service queue.
            *
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked.
            *
            * @return Returns the type, moved out of "cooked" memory and into client storage.
            */
            T get();

            /**
            * @brief The Function Type for the Get And Invoke Operation
            *
            * This is the type of Functor expected for using the getAndInvoke operation. It allows the
            * client to perform an operation on the value dequeued, directly, without having to make
            * perform an additional "move" operation. See getAndInvoke for more details.
            */
            using GetAndInvokeFunctionType = std::function< void( T & ) >;

            /**
            * @brief The Get And Invoke Operation
            *
            * This operation waits for an object to become available for dequeueing and once delivered,
            * Invokes the client provided operation passing it the object. When the client operation has completed,
            * the object is destroyed and its memory block is returned to the "raw" queue for subsequent reuse.
            * This occurs even if the client provided operation .
            *
            * @param operation A value (copy) of the client provided operation.
            *
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked.
            */
            void getAndInvoke( GetAndInvokeFunctionType operation );

            /**
            * @brief The Reserve Put Handle Operation
            *
            * This operation reserves raw memory for a subsequent putOnReserved or an emplaceOnReserved operation.
            * It completes the "First" half of what a put operation does.
            *
            * @note This operation will block if the ObjectQueue is full.
            *
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked.
            *
            * @return Returns a ReservedPutHandle object for a subsequent putOnReserved or an emplaceOnReserved operation.
            * @note This "handle" may be used once and once only. Using it more than once will be silently ignored.
            */
            ReservedPutHandle reservePutHandle();

            /**
            * @brief The Put On Reserved Operation
            *
            * This operation completes the "cooking" process using the handle supplied which contains
            * the "raw" materials. This is the "Second" and "Last" half of the job of put does.
            *
            * @param handle A reference to the ReservedPutHandle previously acquired with a reservePutHandle invocation.
            * @param obj A reference to the object to be enqueued.
            *
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked.
            */
            void putOnReserved( ReservedPutHandle & handle, T & obj );

            /**
            * @brief The Emplace On Reserved Operation
            *
            * This operation completes the "cooking" process using the handle supplied which contains
            * the "raw" materials. This is the "Second" and "Last" half of the job of emplace does.
            *
            * @param handle A reference to the ReservedPutHandle previously acquired with a reservePutHandle invocation.
            * @param args Whatever arguments satisfy a particular constructor overload for type T, possibly including
            * zero arguments.
            *
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked.
            */
            template < typename... Args >
            void emplaceOnReserved( ReservedPutHandle & handle, Args&&... args );


            /**
            * @brief Declare ObjectQueueBase::abort Operation In Public Scope.
            *
            * This declaration brings our base class abort operation into the public scope.
            */
            using ObjectQueueBase::abort;

            /**
            * @brief Declare ObjectQueueBase::getRunningStateStatistics Operation In Public Scope.
            *
            * This declaration brings our base class getRunningStateStatistics operation into the public scope.
            */
            using ObjectQueueBase::getRunningStateStatistics;

        private:
            /**
            * @brief The Abort Reserved Put Operation
            *
            * This operation is for internal usage only by instances of the nested RerveredPutHandle class.
            * If a ReservedPutHandle instance goes out of scope before being utilized with the putOnReserved
            * or the emplaceOnReserved operation. It returns the raw memory back to the rawRingBuffer unless
            * the aborted state is detected.
            *
            * @param pRaw A pointer to raw memory previously obtained by an invocation of the reservePutHandle
            * operation.
            */
            void abortReservedPut( void * pRaw );
        };

    } // end namespace Utility
}

#include "Semaphore.hpp"
#include <thread>

template < typename T >
ReiserRT::Core::ObjectQueue< T >::ObjectQueue( size_t requestedNumElements )
        : ObjectQueueBase( requestedNumElements, paddedTypeAllocSize )
{
}

template < typename T >
ReiserRT::Core::ObjectQueue< T >::~ObjectQueue()
{
    // Invoke abort functionality
    abort();

    // We will wait for 100 milliseconds to help ensure everyone is out of the way of the cooked queue.
    // before we start destroying the left over objects.
    std::this_thread::sleep_for( std::chrono::milliseconds(100) );

    // Setup a lambda function for invoking type T's destructor and invoke the flush operation passing reference to function.
    auto funk = []( void * pV ) noexcept { T * pCooked = reinterpret_cast< T * >( pV ); pCooked->~T(); };
    flush( std::ref( funk ) );
}

template < typename T >
void ReiserRT::Core::ObjectQueue< T >::put( T & obj )
{
    // A Deleter and Managed raw pointer type.
    using DeleterType = std::function< void( void * ) noexcept >;
    using ManagedRawPointerType = std::unique_ptr< void, DeleterType >;

    // Obtain Raw Memory (may block if Raw Queue is empty -> Cooked Queue is full)
    void * pRaw = rawWaitAndGet();

    // Wrap in a managed pointer in case cook move construction throws an exception. We must succeed in putting pointer
    // into cooked queue or returning it to the raw queue or it is leaked forever.
    auto deleter = [ this ]( void * p ) noexcept { this->rawPutAndNotify( p ); };
    ManagedRawPointerType managedRawPtr{ pRaw, std::ref( deleter ) };

    // Cook directly on raw and if construction doesn't throw, release managed pointer's ownership.
    new ( pRaw )T{ std::move( obj ) };
    managedRawPtr.release();

    // Load Cooked Memory (pRaw is cooked now)
    cookedPutAndNotify( pRaw );
}

template < typename T >
template < typename... Args >
void ReiserRT::Core::ObjectQueue< T >::emplace( Args&&... args )
{
    // A Deleter and Managed raw pointer type.
    using DeleterType = std::function< void( void * ) noexcept >;
    using ManagedRawPointerType = std::unique_ptr< void, DeleterType >;

    // Obtain Raw Memory (may block if Raw Queue is empty -> Cooked Queue is full)
    void * pRaw = rawWaitAndGet();

    // Wrap in a managed pointer in case cook move construction throws an exception. We must succeed in putting pointer
    // into cooked queue or returning it to the raw queue or it is leaked forever.
    auto deleter = [ this ]( void * p ) noexcept { this->rawPutAndNotify( p ); };
    ManagedRawPointerType managedRawPtr{ pRaw, std::ref( deleter ) };

    // Cook directly on raw and if construction doesn't throw, release managed pointer's ownership.
    new ( pRaw )T{ std::forward<Args>(args)... };
    managedRawPtr.release();

    // Load Cooked Memory (pRaw is cooked now)
    cookedPutAndNotify( pRaw );
}

template < typename T >
T ReiserRT::Core::ObjectQueue< T >::get()
{
    // A deleter and managed cooked pointer type.
    using DeleterType = std::function< void( T * ) noexcept >;
    using ManagedCookedPointerType = std::unique_ptr< T, DeleterType >;

    // Obtain Cooked Memory (may block if the Cooked Queue is empty)
    T * pCooked = reinterpret_cast< T * >( cookedWaitAndGet() );

    // Wrap in a managed pointer in case exception is thrown when T moved out to return location. We must succeed
    // in replenishing the raw queue or it is leaked forever.
    auto deleter = [ this ]( T * pT ) noexcept { pT->~T(); this->rawPutAndNotify( pT ); };
    ManagedCookedPointerType managedCookedPtr{ pCooked, std::ref( deleter ) };

    // This dereference, dereferences the internal pointer of the managed pointer making it of type "T" (not "T *") which
    // is then "moved out" during the assignment of the return value. However, the managed cooked pointer still owns the pointer
    // and its deleter will be called even though the contents of the encapsulated pointer have been "moved out".
    return std::move( *managedCookedPtr );
}

template < typename T >
void ReiserRT::Core::ObjectQueue< T >::getAndInvoke( GetAndInvokeFunctionType operation )
{
    // A deleter and managed cooked pointer type.
    using DeleterType = std::function< void( T * ) noexcept >;
    using ManagedCookedPointerType = std::unique_ptr< T, DeleterType >;

    // Obtain Cooked Memory (may block if the Cooked Queue is empty)
    T * pCooked = reinterpret_cast< T * >( cookedWaitAndGet() );

    // Wrap in a managed pointer in case an exception is thrown when invoking the user provided operation. We must succeed
    // in replenishing the raw queue or it is leaked forever.
    auto deleter = [ this ]( T * pT ) noexcept { pT->~T(); this->rawPutAndNotify( pT ); };
    ManagedCookedPointerType managedCookedPtr{ pCooked, std::ref( deleter ) };

    // Invoke user provided operation
    operation( *managedCookedPtr );
}

template < typename T >
typename ReiserRT::Core::ObjectQueue< T >::ReservedPutHandle ReiserRT::Core::ObjectQueue< T >::reservePutHandle()
{
    // Obtain Raw Memory (may block if the Raw Queue is empty)
    void * pRaw = rawWaitAndGet();

    // Construct ReservedPutHandle and return
    return ReservedPutHandle{ this, pRaw };
}

template < typename T >
void ReiserRT::Core::ObjectQueue< T >::putOnReserved( ReservedPutHandle & handle, T & obj )
{
    // If the handle is nullptr, throw invalid_argument.
    if ( !handle.pRaw ) throw std::invalid_argument( "ObjectQueue< T >::putOnReserved invoked with invalid handle!!!" );

#if 1
    ///@note The ReservedPutHandle has all the logic required to return the its raw pointer to the raw pool should
    ///we not release it.

    // Cook directly on raw and if construction doesn't throw, we will cache the raw pointer and then release
    // the ReservedPutHandle's pointer's ownership of it.
    new ( handle.pRaw )T{ std::move( obj ) };
    void * pRaw = handle.pRaw;
    handle.pRaw = nullptr;

    // The formerly raw is now cooked. Enqueue it for consumption.
    cookedPutAndNotify( pRaw );
#else
    // A Deleter and Managed raw pointer type.
    using DeleterType = std::function< void( void *& ) noexcept >;
    using ManagedRawPointerType = std::unique_ptr< void, DeleterType >;

    // Wrap in a managed pointer in case cook move construction throws an exception. We must succeed in putting pointer
    // into cooked queue or returning it to the raw queue or it is leaked forever.
    auto deleter = [ this ]( void *& p ) noexcept { this->rawPutAndNotify( p ); p = nullptr; };
    ManagedRawPointerType managedRawPtr{ handle.pRaw, std::ref( deleter ) };

    // Cook directly on raw and if construction doesn't throw, release managed pointer's ownership.
    new ( handle.pRaw )T{ std::move( obj ) };
    managedRawPtr.release();

    // Store off handle's pointer to raw memory and set handle pRaw member to nullptr;
    void * pRaw = handle.pRaw;
    handle.pRaw = nullptr;

    // Load Cooked Memory (pRaw is cooked now)
    cookedPutAndNotify( pRaw );
#endif
}

template < typename T >
template < typename... Args >
void ReiserRT::Core::ObjectQueue< T >::emplaceOnReserved( ReservedPutHandle & handle, Args&&... args )
{
    // If the handle is null pointer, throw invalid_argument.
    if ( !handle.pRaw ) throw std::invalid_argument( "ObjectQueue< T >::putOnReserved invoked with invalid handle!!!" );

#if 1
    ///@note The ReservedPutHandle has all the logic required to return the its raw pointer to the raw pool should
    ///we not release it.

    // Cook directly on raw and if construction doesn't throw, we will cache the raw pointer and then release
    // the ReservedPutHandle's pointer's ownership of it.
    new ( handle.pRaw )T{ std::forward<Args>(args)... };
    void * pRaw = handle.pRaw;
    handle.pRaw = nullptr;

    // The formerly raw is now cooked. Enqueue it for consumption.
    cookedPutAndNotify( pRaw );
#else
    // A Deleter and Managed raw pointer type.
    using DeleterType = std::function< void( void *& ) noexcept >;
    using ManagedRawPointerType = std::unique_ptr< void, DeleterType >;

    // Wrap in a managed pointer in case cook move construction throws an exception. We must succeed in putting pointer
    // into cooked queue or returning it to the raw queue or it is leaked forever.
    auto deleter = [ this ]( void *& p ) noexcept { this->rawPutAndNotify( p ); p = nullptr; };
    ManagedRawPointerType managedRawPtr{ handle.pRaw, std::ref( deleter ) };

    // Cook directly on raw and if construction doesn't throw, release managed pointer's ownership.
    new ( handle.pRaw )T{ std::forward<Args>(args)... };
    managedRawPtr.release();

    // Store off handle raw memory pointer and set handle pRaw member to nullptr;
    void * pRaw = handle.pRaw;
    handle.pRaw = nullptr;

    // Load Cooked Memory (pRaw is cooked now)
    cookedPutAndNotify( pRaw );
#endif
}

template < typename T >
void ReiserRT::Core::ObjectQueue< T >::abortReservedPut( void * pRaw )
{
    // If we are not aborted.
    if ( !isAborted() )
    {
        // Return the raw memory to the raw pool
        rawPutAndNotify( pRaw );
    }
}
#endif /* REISERRT_CORE_OBJECTQUEUE_HPP */
