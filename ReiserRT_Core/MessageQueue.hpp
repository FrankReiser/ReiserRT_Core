/**
* @file MessageQueue.hpp
* @brief The Specification for a Pendable MessageQueue
* @authors Frank Reiser
* @date Created on May 23, 2017
*/

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_

#include "ObjectPool.hpp"
#include "ObjectQueue.hpp"

#include <memory>

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief The Message Queue Abstract Message Base Class
        *
        * All MessageQueue message classes must be derived from this abstract base class.
        * The core requirements for derived messages are specified by its interface functions
        * which are documented within.
        */
        class MessageBase
        {
        public:
            /**
            * @brief Default Constructor for MessageBase
            *
            * This is the default constructor for the MessageBase class. It does nothing of significance
            * and will not throw any exceptions.
            *
            * @note The noexcept clause does not imply any restrictions on derived message types.
            * It is not a requirement.
            */
            MessageBase() noexcept = default;

            /**
            * @brief The Destructor for MessageBase
            *
            * This is the destructor for MessageBase class. It does nothing of significance
            * and will not throw any exceptions. It is declared virtual as this is an abstract base class
            * and is a requirement of our internal ObjectPool.
            *
            * @note The noexcept clause does not imply any restrictions on derived message types.
            * It is not a requirement.
            */
            virtual ~MessageBase() noexcept = default;

            /**
            * @brief The Copy Constructor for MessageBase
            *
            * This is the copy constructor for MessageBase class. It does nothing of significance
            * and will not throw any exceptions.
            *
            * @note The noexcept clause does not imply any restrictions on derived message types.
            * It is not a requirement.
            *
            * @param another A constant reference to another instance of MessageBase in which to copy.
            */
            MessageBase( const MessageBase & another ) noexcept = default;

            /**
            * @brief The Copy Assignment Operator
            *
            * This is the copy assignment operator for MessageBase class. It does nothing of significance
            * and will not throw any exceptions.
            *
            * @note The noexcept clause does not imply any restrictions on derived message types.
            * It is not a requirement.
            *
            * @param another A constant reference to another instance of MessageBase in which to copy assign from.
            */
            MessageBase & operator = ( const MessageBase & another ) noexcept = default;

            /**
            * @brief The Move Constructor for MessageBase
            *
            * This is the move constructor for MessageBase class. It does nothing of significance
            * and will not throw any exceptions.
            *
            * @note The noexcept clause does not imply any restrictions on derived message types.
            * It is not a requirement.
            *
            * @param another A rvalue reference to another instance of MessageBase in which to move contents from.
            */
            MessageBase( MessageBase && another ) noexcept = default;

            /**
            * @brief The Move Assignment Operator
            *
            * This is the move assignment operator for MessageBase class. It does nothing of significance
            * and will not throw any exceptions.
            *
            * @note The noexcept clause does not imply any restrictions on derived message types.
            * It is not a requirement.
            *
            * @param another A rvalue reference to another instance of MessageBase in which to move assign from.
            */
            MessageBase & operator = ( MessageBase && another ) noexcept = default;

            /**
            * @brief The Dispatch Operation
            *
            * This is the abstract dispatch operation. It must be overridden by a derived class to
            * perform the dispatching of the message. It is entirely up to the client to as to what
            * the meaning of "dispatch" is.
            */
            virtual void dispatch() = 0;

            /**
            * @brief The Name Function
            *
            * The name operation is intended to be overridden to provide meaningful information for
            * message dispatching operations. If not overridden, an annoying default is provided.
            *
            * @return Intended to return the name of the message class.
            */
            virtual const char * name() { return "Unforgiven"; }
        };

        /**
        * @brief A MessageQueue Class
        *
        * This class leverages both ObjectPool and ObjectQueue to implement an abstract MessageQueue.
        * It uses ObjectPool for preallocated memory to move enqueued messages into ObjectQueue and
        * ObjectQueue to move smart pointers of abstract messages from input to output where they are
        * dispatched by the getAndDispatch operation.
        *
        * @tparam requestedMaxMessageSize The requested size (in bytes) for message allocation blocks for the internal pool.
        * @note This needs to a minimum of the size of MessageBase and should be the size of the largest derived message.
        * This requested size will be rounded up to next architecture size multiple for alignment purposes.
        */
        template < size_t requestedMaxMessageSize = 0 >
        class MessageQueue
        {
            // Validate requested size is at least the minimum required.
            static_assert( requestedMaxMessageSize >= sizeof( MessageBase ), "Template parameter requestedMaxMessageSize must >= sizeof( MessageQueue::BaseMessage )!!!" );

            /**
            * @brief The Object Pool Type
            *
            * The object pool type is that of our MessageBase. Object Pools support derived message types, which may be larger
            * than MessageBase so we employ the requestedMaxMessageSize template argument here.
            */
            using ObjectPoolType = ReiserRT::Core::ObjectPool< MessageBase, requestedMaxMessageSize >;

            /**
            * @brief The Message Smart Pointer Type
            *
            * We alias our message smart pointer type to that of the ObjectPoolType::ObjectPtrType
            */
            using MessagePtrType = typename ObjectPoolType::ObjectPtrType;

            /**
            * @brief The Object Queue Type
            *
            * The object queue type is that of our MessagePtrType.
            */
            using ObjectQueueType = ReiserRT::Core::ObjectQueue< MessagePtrType >;

            /**
            * @brief The Padded Message Allocation Size
            *
            * We alias our padded message allocation size to that of ObjectPoolType::paddedTypeAllocSize
            * which is the requestedMaxMessageSize rounded up to the next multiple of the architecture size for
            * alignment purposes.
            */
            static constexpr size_t paddedMessageAllocSize = ObjectPoolType::paddedTypeAllocSize;
public:
            /**
            * @brief The Running State Statistics
            *
            * We alias our running state statistics to that our ObjectQueueType::RunningStateStats.
            */
            using RunningStateStats = typename ObjectQueueType::RunningStateStats;

            /**
            * @brief Default Constructor for Message Queue
            *
            * Default construction is disallowed. Hence, this operation is deleted.
            * You must use the qualified constructor which requires that you specify a maximum number of messages
            * that may be enqueued at any point in time
            */
            MessageQueue() = delete;

            /**
            * @brief Qualified Constructor for Message Queue
            *
            * This constructor accepts a requested number of elements argument and constructs an object pool
            * and an object queue passing the argument down.
            *
            * @param requestedNumElements
            */
            explicit MessageQueue( size_t requestedNumElements )
              : objectPool{ requestedNumElements }
              , objectQueue{ requestedNumElements }
              , nameOfLastMessageDispatched{ nullptr }
            {
            }

            /**
            * @brief Destructor for MessageQueue
            *
            * This destructor invokes the object queue's abort operation.
            */
            ~MessageQueue() { objectQueue.abort(); };

            /**
            * @brief Copy Constructor for Message Queue
            *
            * Copy construction is disallowed. Hence, this operation is deleted.
            *
            * @param another A constant reference to another message queue instance.
            */
            MessageQueue( const MessageQueue & another ) = delete;

            /**
            * @brief Copy Assignment Operator for Message Queue
            *
            * Copy assignment is disallowed. Hence, this operation is deleted.
            *
            * @param another A constant reference to another message queue instance.
            */
            MessageQueue & operator =( const MessageQueue & another ) = delete;

            /**
            * @brief Move Constructor for Message Queue
            *
            * Move construction is disallowed. Hence, this operation is deleted.
            *
            * @param another An rvalue reference to another message queue instance.
            */
            MessageQueue( MessageQueue && another ) = delete;

            /**
            * @brief Move Assignment Operator for Message Queue
            *
            * Move assignment is disallowed. Hence, this operation is deleted.
            *
            * @param another An rvalue reference to another message queue instance.
            */
            MessageQueue & operator =( MessageQueue && another ) = delete;

            /**
            * @brief The Put Operation
            *
            * This operation puts (enqueues) a MessageBase derived type into the queue.
            * If the message queue is full at the time of invocation, this operation will block until space becomes available.
            *
            * @tparam T The type of message to enqueue.
            * @note Type T must be derived from MessageBase, must be nothrow move constructible/assignable and nothrow destructable
            * and have a size less than or equal to the padded message allocation block size.
            * @param msg The message to put (enqueue) into the message queue.
            *
            * @throw Throws Semaphore::AbortedException if the ObjectQueue abort operation has been invoked.
            */
            template< typename T >
            void put( T && msg )
            {
                // Type T must be derived from MessageBase.
                static_assert( std::is_base_of< MessageBase, T >::value, "Type T must derived from MessageQueue::BaseMessage!!!" );

                // Type T must be move assignable && move construtible.
                static_assert( std::is_move_constructible<T>::value, "Type T must be move constructible!!!");

                // Type T must be nothrow destructable
                static_assert( std::is_nothrow_destructible<T>::value, "Type T must be no throw destructible!!!" );

                // The sizeof type T must be less than or equal to the paddedMessageAllocSize
                static_assert( sizeof( T ) <= paddedMessageAllocSize,
                                "The sizeof type T must be less than or equal to the paddedMessageAllocSize, derived from requestedMaxMessageSize!!!" );

                // First, we'll reserve a put handle. This will block if the MessageQueue is full serving as a guard before we allocate
                // from the pool which would throw if we allowed it to become exhausted.
                auto reservedPutHandle = objectQueue.reservePutHandle();

                // Now, we should be able to safely get memory from the pool without it throwing an exception.
                // By design, it has, at a minimum, the required number of blocks to meet the internal counted semaphore guard.
                // After the message is moved the pool memory, we'll immediately enqueue it onto the reserved put handle.
                objectQueue.emplaceOnReserved( reservedPutHandle, objectPool.template createObj< T >( std::move( msg ) ) );
            }

            /**
            * @brief The Emplace Operation
            *
            * This operation emplaces (enqueues) a MessageBase derived type into the queue. Emplacing is constructing directly
            * onto the pool memory using a variadic argument list. The arguments are perfect forwarded to the internal createObj call.
            * If the message queue is full at the time of invocation, this operation will block until space becomes available.
            *
            * @tparam T The type of message to enqueue.
            * @note Type T must be derived from MessageBase, must be nothrow move constructible/assignable and nothrow destructable
            * and have a size less than or equal to the padded message allocation block size.
            * @tparam Args A Variadic argument list for constructing in place onto pool memory.
            * @param args The arguments require to construct a message to enqueue into the message queue.
            *
            * @throw Throws Semaphore::AbortedException if the ObjectQueue abort operation has been invoked.
            */
            template < typename T, typename... Args >
            void emplace( Args&&... args )
            {
                // Type T must be derived from MessageBase.
                static_assert( std::is_base_of< MessageBase, T >::value, "Type T must derived from MessageQueue::BaseMessage!!!" );

                // Type T must be move assignable && move constructible.
                static_assert( std::is_move_constructible<T>::value, "Type T must be move constructible!!!");

                // Type T must be nothrow destructable
                static_assert( std::is_nothrow_destructible<T>::value, "Type T must be no throw destructable!!!" );

                // The sizeof type T must be less than or equal to the paddedMessageAllocSize
                static_assert( sizeof( T ) <= paddedMessageAllocSize,
                                "The sizeof type T must be less than or equal to the paddedMessageAllocSize, derived from requestedMaxMessageSize!!!" );

                // First, we'll reserve a put handle. This will block if the MessageQueue is full serving as a guard before we allocate
                // from the pool which would throw if we allowed it to become exhausted.
                auto reservedPutHandle = objectQueue.reservePutHandle();

                // Now, we should be able to safely get memory from the pool without it throwing an exception.
                // By design, it has, at a minimum, the required number of blocks to meet the internal counted semaphore guard.
                // After the message is emplaced onto pool memory, we'll imediately enqueue it onto the reserved put handle.
                objectQueue.emplaceOnReserved( reservedPutHandle, objectPool.template createObj< T >( std::forward<Args>(args)...  ) );
            }

            /**
            * @brief The Get and Dispatch Operation
            *
            * This operation waits (blocks) until a message is available in the queue. As soon as a message is available
            * for dequeuing, it is retrieved and directly dispatched by invoking the abstract MessageBase::dispatch operation.
            * It utilizes the ObjectQueue::getAndInvoke operation which guarantees that an exception thrown during the dispatch,
            * leaves our internal queue sane (invariant). However, such an exception would propagate up the call stack.
            *
            * @throw Throws Semaphore::AbortedException if the ObjectQueue abort operation has been invoked.
            */
            void getAndDispatch()
            {
                // Setup a lambda function for invoking the message dispatch operation.
                auto funk = [ this ]( MessagePtrType & msgPtr )
                {
                    nameOfLastMessageDispatched = msgPtr->name();
                    msgPtr->dispatch();
                };

                // Get (wait) for a message and invoke our lambda via reference.
                // If the dispatch throws anything, it will propagate up the call stack.
                objectQueue.getAndInvoke( std::ref( funk ) );
            }

            /**
            * @brief Wake-up Call Function Type
            *
            * This type represents the signature of a function accepting no arguments and returning nothing (void).
            * It is used by the getAndDispactch operation that notifies the caller when a message is about to be dispatched.
            */
            using WakeupCallFunctionType = std::function< void() >;

            /**
            * @brief The Get and Dispatch Operation with Wake-up Notification
            *
            * This operation waits (blocks) until a message is available in the queue. As soon as a message is available
            * for dequeuing, the wake-up function is invoked and the message is retrieved and directly dispatched
            * by invoking the abstract MessageBase::dispatch operation.
            * It utilizes the ObjectQueue::getAndInvoke operation which guarantees that an exception thrown during the dispatch,
            * leaves our internal queue sane (invariant). However, such an exception would propagate up the call stack.
            *
            * @param wakeupFunctor A call-able object to be invoked upon message availability.
            * @throw Throws Semaphore::AbortedException if the ObjectQueue abort operation has been invoked.
            */
            void getAndDispatch( WakeupCallFunctionType wakeupFunctor )
            {
                // Setup a lambda function for invoking the message dispatch operation.
                auto funk = [ this, &wakeupFunctor ]( MessagePtrType & msgPtr )
                {
                    wakeupFunctor();
                    nameOfLastMessageDispatched = msgPtr->name();
                    msgPtr->dispatch();
                };

                // Get (wait) for a message and invoke our lambda via reference.
                // If the dispatch throws anything, it will propagate up the call stack.
                objectQueue.getAndInvoke( std::ref( funk ) );
            }

            /**
            * @brief Get the Name of the Last Message Dispatched.
            *
            * This is more of a diagnostic routine which may be helpful if one particular message type is bombing on dispatch.
            * In order to be useful, the MessageBase::name abstract should be overridden to provide something other than the default
            * implementation. Please see MessageBase::name default implementation for an example of how to implement an override.
            *
            * @return Returns the name of the last message dispatched.
            */
            const char * getNameOfLastMessageDispatched() { return nameOfLastMessageDispatched; }

            /**
            * @brief The Abort Operation
            *
            * This operation defers to ObjectQueue::abort to abort the message queue.
            */
            void abort() { objectQueue.abort(); }

            /**
            * @brief The Get Running State Statistics.
            *
            * This operation returns the running state statistics. This yields a size, high water mark and running count.
            *
            * @return Returns running state statistics.
            */
            RunningStateStats getRunningStateStatistics() noexcept
            {
                return objectQueue.getRunningStateStatistics();
            }

        private:
            /**
            * @brief The Object Pool
            *
            * This attribute maintains the state of our object pool.
            */
            ObjectPoolType objectPool;

            /**
            * @brief The Object Queue
            *
            * This attribute maintains the state of our object queue.
            */
            ObjectQueueType objectQueue;

            /**
            * @brief The Name of the Last Message Dispatched
            *
            * This attribute maintains the name of the last message dispatched by the MessageQueue.
            */
            const char * nameOfLastMessageDispatched;
        };

    }
}



#endif /* MESSAGEQUEUE_H_ */
