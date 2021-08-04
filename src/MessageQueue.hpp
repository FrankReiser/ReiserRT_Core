/**
* @file MessageQueue.hpp
* @brief The Specification for a Pendable MessageQueue
* @authors Frank Reiser
* @date Created on May 23, 2017
*/

#ifndef REISERRT_CORE_MESSAGEQUEUE_HPP
#define REISERRT_CORE_MESSAGEQUEUE_HPP

#include "ReiserRT_CoreExport.h"

#include "MessageQueueBase.hpp"
#include "ReiserRT_CoreExceptions.hpp"

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief A MessageQueue Class
        *
        * This class leverages both ObjectPool and ObjectQueue to implement an abstract MessageQueue.
        * It uses ObjectPool for preallocated memory to move enqueued messages into ObjectQueue and
        * ObjectQueue to move smart pointers of abstract messages from input to output where they are
        * dispatched by the getAndDispatch operation.
        */
        class ReiserRT_Core_EXPORT MessageQueue : public MessageQueueBase {
        public:
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
            * This constructor accepts a requested number of elements, maximum element size and
            * a flag stating whether or not to enable dispatch locking.
            *
            * @param requestedNumElements The Requested Message Queue Depth
            * @param requestedMaxMessageSize The Requested Maximum Message Size for Derived Message Types.
            * There is no default value for this parameter. You must specify your maximum, derived message size.
            * @param enableDispatchLocking Set to true to enable the dispatch locking capability. Dispatch locking
            * allows execution serialization between synchronous client threads and an asynchronous dispatch loop
            * thread. By default this feature is disabled as there is a small performance penalty that a dispatch
            * loop must pay to support it. If a client must coordinate synchronous and asynchronous activity,
            * then a client should enable this feature. This cannot be changed after construction.
            */
            explicit MessageQueue(size_t requestedNumElements, size_t requestedMaxMessageSize,
                bool enableDispatchLocking = false );

            /**
            * @brief Destructor for MessageQueue
            *
            * This destructor is defaulted. Our base constructor takes care of everything.
            */
            ~MessageQueue() = default;

            /**
            * @brief Copy Constructor for Message Queue
            *
            * Copy construction is disallowed. Hence, this operation is deleted.
            *
            * @param another A constant reference to another message queue instance.
            */
            MessageQueue(const MessageQueue &another) = delete;

            /**
            * @brief Copy Assignment Operator for Message Queue
            *
            * Copy assignment is disallowed. Hence, this operation is deleted.
            *
            * @param another A constant reference to another message queue instance.
            */
            MessageQueue &operator=(const MessageQueue &another) = delete;

            /**
            * @brief Move Constructor for Message Queue
            *
            * Move construction is disallowed. Hence, this operation is deleted.
            *
            * @param another An rvalue reference to another message queue instance.
            */
            MessageQueue(MessageQueue &&another) = delete;

            /**
            * @brief Move Assignment Operator for Message Queue
            *
            * Move assignment is disallowed. Hence, this operation is deleted.
            *
            * @param another An rvalue reference to another message queue instance.
            */
            MessageQueue &operator=(MessageQueue &&another) = delete;

            /**
            * @brief The Put Operation
            *
            * This operation puts (enqueues) a MessageBase derived type into the queue.
            * If the message queue is full at the time of invocation, this operation will block until space becomes available.
            *
            * @tparam M The type of message to enqueue.
            * @note Type M must be derived from MessageBase, must be nothrow move constructible/assignable and nothrow destructable
            * and have a size less than or equal to the padded message allocation block size managed by its internal
            * ObjectPool instance.
            * @param msg The message to put (enqueue) into the message queue.
            *
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation has been invoked.
            * @throw Throws ReiserRT::Core::MessageQueueElementSizeError if the derived message type exceeds that
            * allowed by the requestedMaxMessageSize specified during construction plus any padding that
            * may have been added for alignment purposes.
            */
            template<typename M>
            void put(M &&msg) {
                // Type M must be derived from MessageBase.
                static_assert(std::is_base_of<MessageBase, M>::value,
                              "Type M must derived from MessageQueue::BaseMessage!!!");

                // Type M must be move assignable && move constructible.
                static_assert(std::is_move_constructible<M>::value, "Type M must be move constructible!!!");

                // Type M must be nothrow destructible
                static_assert(std::is_nothrow_destructible<M>::value, "Type M must be no throw destructible!!!");

                // Before we even bother getting a raw block of memory, we will validate that
                // the type being created will fit in an element block.
                if ( getElementSize() < sizeof( M ) )
                    throw MessageQueueElementSizeError( "MessageQueue::put: The size of type M exceeds maximum element size" );

                // Wait on raw memory availability
                void * pRaw = rawWaitAndGet();

                // Wrap it up for safe return should we throw an exception when we cook it.
                RawMemoryManager rawMemoryManager{ this, pRaw };

                // Cook raw memory
                M * pM = new( pRaw )M{ std::forward<M>(msg) };

                // If here, raw is successfully cooked. Release RawMemoryManager from cleanup responsibility and
                // send cooked out for delivery.
                rawMemoryManager.release();
                cookedPutAndNotify( pM );
            }

            /**
            * @brief The Emplace Operation
            *
            * This operation emplaces (enqueues) a MessageBase derived type into the queue. Emplacing is constructing directly
            * onto the pool memory using a variadic argument list. The arguments are perfect forwarded to the internal createObj call.
            * If the message queue is full at the time of invocation, this operation will block until space becomes available.
            *
            * @tparam M The type of message to enqueue.
            * @note Type M must be derived from MessageBase, must be nothrow move constructible/assignable and nothrow destructable
            * and have a size less than or equal to the padded message allocation block size managed by its internal
            * ObjectPool instance.
            * @tparam Args A Variadic argument list for constructing in place onto pool memory.
            * @param args The arguments require to construct a message to enqueue into the message queue.
            *
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation has been invoked.
            * @throw Throws ReiserRT::Core::MessageQueueElementSizeError if the derived message type exceeds that
            * allowed by the requestedMaxMessageSize specified during construction plus any padding that
            * may have been added for alignment purposes.
            */
            template<typename M, typename... Args>
            void emplace(Args &&... args) {
                // Type M must be derived from MessageBase.
                static_assert(std::is_base_of<MessageBase, M>::value,
                              "Type M must derived from MessageQueue::BaseMessage!!!");

                // Type M must be move assignable && move constructible.
                static_assert(std::is_move_constructible<M>::value, "Type M must be move constructible!!!");

                // Type M must be nothrow destructible
                static_assert(std::is_nothrow_destructible<M>::value, "Type M must be no throw destructible!!!");

                // Before we even bother getting a raw block of memory, we will validate that
                // the type being created will fit in an element block.
                if ( getElementSize() < sizeof( M ) )
                    throw MessageQueueElementSizeError( "MessageQueue::emplace: The size of type M exceeds maximum element size" );

                // Wait on raw memory availability
                void * pRaw = rawWaitAndGet();

                // Wrap it up for safe return should we throw an exception when we cook it.
                RawMemoryManager rawMemoryManager{ this, pRaw };

                // Cook raw memory
                M * pM = new( pRaw )M{ std::forward<Args>(args)... };

                // If here, raw is successfully cooked. Release RawMemoryManager from cleanup responsibility and
                // send cooked out for delivery.
                rawMemoryManager.release();
                cookedPutAndNotify( pM );
            }

            /**
            * @brief The Get and Dispatch Operation
            *
            * This operation waits (blocks) until a message is available in the queue. As soon as a message is available
            * for de-queuing, the dispatch lock is taken and the message is dispatched via the
            * MessageBase::dispatch operation.
            *
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation has been invoked.
            */
            void getAndDispatch();

            /**
            * @brief The Get and Dispatch Operation with Wake-up Notification
            *
            * This operation waits (blocks) until a message is available in the queue. As soon as a message is available
            * for dequeuing, the wakeupFunctor is invoked and then the dispatch lock is taken.
            * The message is then dispatched via the MessageBase::dispatch operation.
            *
            * @param wakeupFunctor A call-able object to be invoked upon message availability.
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the abort operation has been invoked.
            */
            void getAndDispatch(WakeupCallFunctionType wakeupFunctor);

            /**
            * @brief Get the Name of the Last Message Dispatched.
            *
            * This is more of a diagnostic routine which may be helpful if one particular message type is bombing on dispatch.
            * In order to be useful, the MessageBase::name abstract should be overridden to provide something other than the default
            * implementation. Please see MessageBase::name default implementation for an example of how to implement an override.
            *
            * @return Returns the name of the last message dispatched.
            */
            using MessageQueueBase::getNameOfLastMessageDispatched;

            /**
            * @brief The Abort Operation
            *
            * This operation defers to ObjectQueue::abort to abort the message queue.
            */
            using MessageQueueBase::abort;

            /**
            * @brief The Get Running State Statistics.
            *
            * This operation defers to MessageQueueBase::getRunningStateStatistics to obtain statistics.
            *
            * @return Returns running state statistics.
            */
            using MessageQueueBase::getRunningStateStatistics;

            /**
            * @brief The Get Auto Dispatch Lock Operation
            *
            * This operation defers to MessageQueueBase::getAutoDispatchLock to obtain an AutoDispatchLock object.
            *
            * @throw Throws ReiserRT::Core::MessageQueueDispatchLockingDisabled if dispatch locking was not explicitly
            * enabled during construction.
            */
            using MessageQueueBase::getAutoDispatchLock;
        };

    }
}

#endif /* REISERRT_CORE_MESSAGEQUEUE_HPP */
