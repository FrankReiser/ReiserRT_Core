/**
* @file MessageQueue.hpp
* @brief The Specification for a Pendable MessageQueue
* @authors Frank Reiser
* @date Created on May 23, 2017
*/

#ifndef REISERRT_CORE_MESSAGEQUEUE_HPP
#define REISERRT_CORE_MESSAGEQUEUE_HPP

#include "ReiserRT_CoreExport.h"

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
        class ReiserRT_Core_EXPORT MessageBase
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
            virtual const char * name() const;
        };

        /**
        * @brief A MessageQueue Class
        *
        * This class leverages both ObjectPool and ObjectQueue to implement an abstract MessageQueue.
        * It uses ObjectPool for preallocated memory to move enqueued messages into ObjectQueue and
        * ObjectQueue to move smart pointers of abstract messages from input to output where they are
        * dispatched by the getAndDispatch operation.
        */
        class ReiserRT_Core_EXPORT MessageQueue {
            /**
            * @brief The Object Pool Type
            *
            * The object pool type is that of our MessageBase. Object Pools support derived message types, which may be larger
            * than MessageBase.
            */
            using ObjectPoolType = ReiserRT::Core::ObjectPool<MessageBase>;

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
            using ObjectQueueType = ReiserRT::Core::ObjectQueue<MessagePtrType>;

        public:
            /**
            * @brief The Running State Statistics
            *
            * We alias our running state statistics to that our ObjectQueueType::RunningStateStats.
            */
            using RunningStateStats = typename ObjectQueueType::RunningStateStats;

            /**
            * @brief Forward Declaration for Details Class
            *
            * The Details class contains implementation state data which is not exposed at the interface layer.
            */
            class Details;

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
            * and an object queue passing the arguments down.
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
            * This destructor invokes the object queue's abort operation.
            */
            ~MessageQueue();

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
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked or if the
            * derived message type exceeds that allowed by its internal ObjectPool instance.
            */
            template<typename M>
            void put(M &&msg) {
                // Type M must be derived from MessageBase.
                static_assert(std::is_base_of<MessageBase, M>::value,
                              "Type M must derived from MessageQueue::BaseMessage!!!");

                // Type M must be move assignable && move construtible.
                static_assert(std::is_move_constructible<M>::value, "Type M must be move constructible!!!");

                // Type M must be nothrow destructable
                static_assert(std::is_nothrow_destructible<M>::value, "Type M must be no throw destructible!!!");

#if 0
                // The sizeof type M must be less than or equal to the paddedMessageAllocSize
                static_assert( sizeof( M ) <= paddedMessageAllocSize,
                                "The sizeof type M must be less than or equal to the paddedMessageAllocSize, derived from requestedMaxMessageSize!!!" );
#endif

                // First, we'll reserve a put handle. This will block if the MessageQueue is full serving as a guard before we allocate
                // from the pool which would throw if we allowed it to become exhausted.
                auto reservedPutHandle = objectQueue.reservePutHandle();

                // Now, we should be able to safely get memory from the pool without it throwing an exception.
                // By design, it has, at a minimum, the required number of blocks to meet the internal counted semaphore guard.
                // After the message is moved the pool memory, we'll immediately enqueue it onto the reserved put handle.
                objectQueue.emplaceOnReserved(reservedPutHandle, objectPool.createObj<M>(std::forward<M>(msg)));
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
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked or if the
            * derived message type exceeds that allowed by its internal ObjectPool instance.
            */
            template<typename M, typename... Args>
            void emplace(Args &&... args) {
                // Type M must be derived from MessageBase.
                static_assert(std::is_base_of<MessageBase, M>::value,
                              "Type M must derived from MessageQueue::BaseMessage!!!");

                // Type M must be move assignable && move constructible.
                static_assert(std::is_move_constructible<M>::value, "Type M must be move constructible!!!");

                // Type M must be nothrow destructable
                static_assert(std::is_nothrow_destructible<M>::value, "Type M must be no throw destructable!!!");

#if 0
                // The sizeof type MT must be less than or equal to the paddedMessageAllocSize
                static_assert( sizeof( M ) <= paddedMessageAllocSize,
                                "The sizeof type M must be less than or equal to the paddedMessageAllocSize, derived from requestedMaxMessageSize!!!" );
#endif

                // First, we'll reserve a put handle. This will block if the MessageQueue is full serving as a guard before we allocate
                // from the pool which would throw if we allowed it to become exhausted.
                auto reservedPutHandle = objectQueue.reservePutHandle();

                // Now, we should be able to safely get memory from the pool without it throwing an exception.
                // By design, it has, at a minimum, the required number of blocks to meet the internal counted semaphore guard.
                // After the message is emplaced onto pool memory, we'll immediately enqueue it onto the reserved put handle.
                objectQueue.emplaceOnReserved(reservedPutHandle, objectPool.createObj<M>(std::forward<Args>(args)...));
            }

            /**
            * @brief The Get and Dispatch Operation
            *
            * This operation waits (blocks) until a message is available in the queue. As soon as a message is available
            * for dequeuing, the dispatch lock is taken and the message is dispatched via the
            * MessageBase::dispatch operation.
            *
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked.
            */
            void getAndDispatch();

            /**
            * @brief Wake-up Call Function Type
            *
            * This type represents the signature of a function accepting no arguments and returning nothing (void).
            * It is used by the getAndDispatch operation that notifies the caller when a message is about to be dispatched.
            */
            using WakeupCallFunctionType = std::function<void()>;

            /**
            * @brief The Get and Dispatch Operation with Wake-up Notification
            *
            * This operation waits (blocks) until a message is available in the queue. As soon as a message is available
            * for dequeuing, the wakeupFunctor is invoked and then the dispatch lock is taken.
            * The message is then dispatched via the MessageBase::dispatch operation.
            *
            * @param wakeupFunctor A call-able object to be invoked upon message availability.
            * @throw Throws std::runtime_error if the ObjectQueue abort operation has been invoked.
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
            const char *getNameOfLastMessageDispatched();

            /**
            * @brief The Abort Operation
            *
            * This operation defers to ObjectQueue::abort to abort the message queue.
            */
            void abort();

            /**
            * @brief The Get Running State Statistics.
            *
            * This operation returns the running state statistics. This yields a size, high water mark and running count.
            *
            * @return Returns running state statistics.
            */
            RunningStateStats getRunningStateStatistics() noexcept;

            /**
            * brief A Dispatch Lock Automatic Lock/Release Mechanism
            *
            * MessageQueue provides the ability to dispatch messages on an independent message queue
            * processing thread of any particular client using a MessageQueue, asynchronously.
            * However, that client may have also have synchronous processing requirements.
            * This class affords a client the ability to do both through a common locking mechanism,
            * thereby affording a synchronization means between asynchronous message dispatch implementations and other
            * synchronous requirements.
            * @note To utilize dispatch locking, it must be enabled during MessageQueue construction.
            */
            class AutoDispatchLock
            {
            private:
                /**
                * @brief Friend Declaration
                *
                * Only MessageQueue may construct instances of this class.
                */
                friend class MessageQueue;

                /**
                * @brief Constructor for AutoDispatchLock.
                *
                * This Constructor takes the dispatch lock.
                *
                * @param pTheDetails
                */
                explicit AutoDispatchLock( Details * pTheDetails );

            public:
                /**
                * @brief Default Construction
                *
                * Details attribute will be set to nullptr;
                */
                AutoDispatchLock() = delete;

                /**
                * @brief
                *
                * This destructor release the dispatch lock.
                */
                ~AutoDispatchLock();

                /**
                * @brief Copy Constructor Deleted
                *
                * Copy construction is disallowed and is deleted.
                *
                * @param another Another instance of an AutoDispatchLock
                */
                AutoDispatchLock( const AutoDispatchLock & another ) = delete;

                /**
                * @brief Copy Assignment Deleted
                *
                * Copy assignment is disallowed and is deleted.
                *
                * @param another Another instance of an AutoDispatchLock
                */
                AutoDispatchLock & operator =( const AutoDispatchLock & another ) = delete;

                /**
                * @brief Move Construction
                *
                * Move construction transfers ownership of details.
                * @note A default move constructor does not guarantee this. It is critical the moved from gets
                * assigned null.
                *
                * @param another Another instance of an AutoDispatchLock
                */
                AutoDispatchLock( AutoDispatchLock && another ) noexcept
                  : pDetails{ another.pDetails }
                {
                    another.pDetails = nullptr;
                }

                /**
                * @brief Move Assignment
                *
                * Move assignment transfers ownership of details.
                * @note A default move assignment operator does not guarantee this. It is critical the moved from gets
                * assigned null.
                *
                * @param another Another instance of an AutoDispatchLock
                */
                AutoDispatchLock & operator =( AutoDispatchLock && another ) noexcept
                {
                    if (this != &another)
                    {
                        pDetails = another.pDetails;
                        another.pDetails = nullptr;
                    }
                    return *this;
                }

            private:
                /**
                * @brief MessageQueue Details Reference.
                *
                * A pointer reference to the hidden MessageQueue details.
                */
                Details * pDetails;
            };

            /**
            * @brief Obtain a AutoDispatchLock Object
            *
            * Returns an automatic dispatch lock object. The dispatch lock is taken on construction of AutoDispatchLock.
            * When the AutoDispatchLock object is destroy, the dispatch lock is released.
            *
            * @throw Throws std::runtime_error if dispatch locking was not enabled during MessageQueue construction.
            * You must enable dispatch locking during construction of MessageQueue to use this feature.
            *
            * @return Returns an instance of AutoDispatchLock.
            */
            AutoDispatchLock getAutoDispatchLock();

        private:
            /**
            * @brief MessageQueue Details Reference.
            *
            * A pointer reference to the hidden MessageQueue details.
            */
            Details * pDetails;

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
        };

    }
}

#endif /* REISERRT_CORE_MESSAGEQUEUE_HPP */
