/**
* @file MessageQueueBase.hpp
* @brief The Specification file for MessageQueueBase
*
* This file came into existence in a effort to replace an ObjectPool usage and ObjectQueue implementation
* in an effort to eliminate an extra Mutex take/release on enqueueing and de-queueing of messages.
* ObjectQueue is being eliminated in this process.
*
* @authors Frank Reiser
* @date Created on Apr 6, 2021
*/

#ifndef REISERRT_CORE_MESSAGEQUEUEBASE_HPP
#define REISERRT_CORE_MESSAGEQUEUEBASE_HPP

#include "ReiserRT_CoreExport.h"
#include "Mutex.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

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
            * This is not a requirement.
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
            * This is not a requirement.
            */
            virtual ~MessageBase() noexcept = default;

            /**
            * @brief The Copy Constructor for MessageBase
            *
            * This is the copy constructor for MessageBase class. It does nothing of significance
            * and will not throw any exceptions.
            *
            * @note The noexcept clause does not imply any restrictions on derived message types.
            * This is not a requirement.
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
            * This is not a requirement.
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
            * This is not a requirement.
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
            * This is not a requirement.
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
         * @brief The MessageQueueBase Class
         *
         * This class provides the interface operations for a hidden implementation. It's API is largely
         * "protected" as it is only intended to be used directly by MessageQueue and serves as a direct base.
         */
        class ReiserRT_Core_EXPORT MessageQueueBase
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
            * @brief Wake-up Call Function Type
            *
            * This type represents the signature of a function accepting no arguments and returning nothing (void).
            * It is used by the getAndDispatch operation that notifies the caller when a message is about to be dispatched.
            */
            using WakeupCallFunctionType = std::function<void()>;

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
            * brief A Dispatch Lock Automatic Lock/Release Mechanism
            *
            * MessageQueue provides the ability to dispatch messages on an independent
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
                * Only MessageQueueBase may construct initial instances of this class. After that,
                * instances may be moved via the public move constructor.
                */
                friend class MessageQueueBase;

                /**
                * @brief Constructor for AutoDispatchLock.
                *
                * This Constructor takes a dispatch lock. It is release when the destructor is called.
                *
                * @param pTheMQB The instance of MessageQueueBase which invoked the constructor.
                */
                explicit AutoDispatchLock( MessageQueueBase * pTheMQB );

            public:
                /**
                * @brief Default Construction Deleted
                *
                * Default construction is disallowed. This operation has been deleted.
                */
                AutoDispatchLock() = delete;

                /**
                * @brief Destructor
                *
                * This destructor release the dispatch lock if we have not been "moved from".
                */
                ~AutoDispatchLock();

                /**
                * @brief Copy Constructor Deleted
                *
                * Copy construction is disallowed. This operation has been deleted.
                *
                * @param another Another instance of an AutoDispatchLock
                */
                AutoDispatchLock( const AutoDispatchLock & another ) = delete;

                /**
                * @brief Copy Assignment Deleted
                *
                * Copy assignment is disallowed. This operation has been deleted.
                *
                * @param another Another instance of an AutoDispatchLock
                */
                AutoDispatchLock & operator =( const AutoDispatchLock & another ) = delete;

                /**
                * @brief Move Construction
                *
                * Move construction transfers ownership of the MessageQueueBase reference.
                * @note A default move constructor does not guarantee this. It is critical the moved from gets
                * assigned null.
                *
                * @param another Another instance of an AutoDispatchLock
                */
                AutoDispatchLock( AutoDispatchLock && another ) noexcept : pMQB{ another.pMQB }, locked{ another.locked }
                {
                    another.pMQB = nullptr;
                    another.locked = 0;
                }

                /**
                * @brief Move Assignment
                *
                * Move assignment transfers ownership of the MessageQueueBase reference.
                * @note A default move assignment operator does not guarantee this. It is critical the moved from gets
                * assigned null.
                *
                * @param another Another instance of an AutoDispatchLock
                */
                AutoDispatchLock & operator =( AutoDispatchLock && another ) noexcept
                {
                    if (this != &another)
                    {
                        pMQB = another.pMQB;
                        locked = another.locked;
                        another.pMQB = nullptr;
                        another.locked = 0;
                    }
                    return *this;
                }

                /**
                * @brief Lock Operation
                *
                * This operation provides for more nuanced control of the dispatch lock should it be necessary.
                * It provides for part of the "Basic Lockable" duck type interface and may be utilized like std::unique_lock.
                */
                void lock();

                /**
                * @brief Unlock Operation
                *
                * This operation provides for more nuanced control of the dispatch lock should it be necessary.
                * It provides for part of the "Basic Lockable" duck type interface and may be utilized like std::unique_lock.
                */
                void unlock();

                /**
                * @brief Alias for Native Type
                *
                * This is simply an alias for our native type. If we are operating under POSIX PTHREADS, this
                * is a pthread_mutex_t pointer.
                */
                using NativeHandleType = std::mutex::native_handle_type;

                /**
                * @brief The Native Handle Operation
                *
                * This operation will return the address of our native handle type.
                *
                * @return Returns the address of our encapsulated native mutex object.
                */
                NativeHandleType native_handle();

            private:
                /**
                * @brief MessageQueueBase Reference.
                *
                * A pointer to the MessageQueueBase.
                */
                MessageQueueBase * pMQB{};
                size_t locked{1};
            };

        protected:
            using FlushingFunctionType = std::function< void( void * ) noexcept >;

            /**
            * @brief A Memory Manager for Raw Memory.
            *
            * This class exists to ensure that no leakage of the raw memory occurs during "cooking" operations.
            * Whether "cooking" succeeds without throwing or not, raw memory is returned to the
            * raw queue within the implementation if not explicitly released.
            */
            struct RawMemoryManager
            {
                /**
                * @brief Constructor for RawMemoryManager.
                *
                * This constructor records both an instance of the MessageQueueBase object that constructed us
                * and a pointer to raw memory.
                *
                * @param pMQBase A pointer to the MessageQueueBase object that instantiated us.
                * @param pTheRaw A pointer to the raw memory to return to the raw queue.
                */
                RawMemoryManager( MessageQueueBase * pMQBase, void * pTheRaw ) : pMQB{ pMQBase }, pR{ pTheRaw } {}

                /**
                * @brief Destructor for RawMemoryManager.
                *
                * This destructor returns raw memory back to the raw queue unless it has been formally released
                * through the `release` operation.
                */
                ~RawMemoryManager() { if ( pMQB && pR ) pMQB->rawPutAndNotify( pR ); }

                /**
                * @brief Copy Constructor for RawMemoryManager Disallowed
                *
                * Copy construction is disallowed. Hence, this operation is deleted.
                *
                * @param another A reference to another RawMemoryManager instance.
                */
                RawMemoryManager( RawMemoryManager & another ) = delete;

                /**
                * @brief Copy Assignment Operator for RawMemoryManager Disallowed
                *
                * Copy assignment is disallowed. Hence, this operation is deleted.
                *
                * @param another A reference to another RawMemoryManager instance.
                */
                RawMemoryManager & operator=( RawMemoryManager & another ) = delete;

                /**
                * @brief The Release Operation
                *
                * This operation releases RawMemoryManager from responsibility of needing to return
                * raw memory back to the raw queue.
                */
                void release() { pR = nullptr; }

            private:
                /**
                * @brief A Reference to the MessageQueueBase that constructed us.
                *
                * This attribute maintains a reference to the MessageQueueBase object that constructed us.
                */
                MessageQueueBase * pMQB;

                /**
                * @brief A Reference to the raw memory that we are managing.
                *
                * This attribute maintains a reference to the raw memory that we are managing.
                */
                void * pR;
            };

            /**
            * @brief A Memory Manager for Cooked Memory.
            *
            * This class exists to guarantee that no leakage of  memory occurs during dispatch operations.
            * Whether dispatch succeeds without throwing or not, cooked memory is destroyed and returned to the
            * raw queue within the implementation.
            */
            struct CookedMemoryManager
            {
                /**
                * @brief Constructor for CookedMemoryManager.
                *
                * This constructor records both an instance of the MessageQueueBase object that constructed us
                * and a pointer to "cooked" memory (i.e., a pointer to a MessageBase).
                *
                * @param pMQBase A pointer to the MessageQueueBase object that instantiated us.
                * @param pMBase A pointer to the MessageBase to destroy and return to the raw queue.
                */
                CookedMemoryManager( MessageQueueBase * pMQBase, MessageBase * pMBase ) : pMQB{ pMQBase }, pMsg{ pMBase } {}

                /**
                * @brief Destructor for CookedMemoryManager.
                *
                * This destructor invokes the destructor of the MessageBase object it is mantaining
                * and returns the formerly "cooked" MessageBase memory back to the raw queue.
                */
                ~CookedMemoryManager() { if ( pMQB && pMsg ) { pMsg->~MessageBase(); pMQB->rawPutAndNotify( pMsg ); } }

                /**
                * @brief Copy Constructor for CookedMemoryManager Disallowed
                *
                * Copy construction is disallowed. Hence, this operation is deleted.
                *
                * @param another A reference to another CookedMemoryManager instance.
                */
                CookedMemoryManager( CookedMemoryManager &&another ) = delete;

                /**
                * @brief Copy Assignment Operator for CookedMemoryManager Disallowed
                *
                * Copy assignment is disallowed. Hence, this operation is deleted.
                *
                * @param another A reference to another CookedMemoryManager instance.
                */
                CookedMemoryManager &operator=( CookedMemoryManager &&another ) = delete;

            private:
                /**
                * @brief A Reference to the MessageQueueBase that constructed us.
                *
                * This attribute maintains a reference to the MessageQueueBase object that constructed us.
                */
                MessageQueueBase * pMQB;

                /**
                * @brief A Reference to the MessageBase that we are managing.
                *
                * This attribute maintains a reference to the MessageBase that we are managing.
                */
                MessageBase * pMsg;
            };

            /**
            * @brief Qualified Constructor for MessageQueueBase
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
            explicit MessageQueueBase( std::size_t requestedNumElements, std::size_t requestedMaxMessageSize,
                                       bool enableDispatchLocking );

            /**
             * @brief Destructor for MessageQueueBase
             *
             * This operation aborts the implementation's ring buffers and any pending get/put operations.
             * It then flushes out and destroys any MessageBaseObjects in the "cooked" queue.
             */
            ~MessageQueueBase();

            /**
            * @brief The Get Running State Statistics Operation
            *
            * This operation provides for performance monitoring of the MessageQueueBase.
            *
            * @return Returns a snapshot of RunningStateStats.
            */
            RunningStateStats getRunningStateStatistics() noexcept;

            /**
            * @brief The abort Operation
            *
            * This operation is invoked to abort the MessageQueueBase.
            *
            * @note Once aborted, there is no recovery. Put, emplace or get operations will receive a std::runtime_error.
            */
            void abort();

            /**
            * @brief The rawWaitAndGet Operation
            *
            * This operation is invoked to obtain raw memory from the raw implementation.
            *
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the "raw" ring buffer becomes aborted.
            *
            * @return Returns a void pointer to raw memory from the implementation.
            */
            void * rawWaitAndGet();

            /**
            * @brief The cookedPutAndNotify Operation
            *
            * This operation loads cooked memory into the implementation and notifies any waiters of availability.
            *
            * @param pCooked A pointer to an abstract MessageBase object.
            *
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the "cooked" ring buffer becomes aborted.
            */
            void cookedPutAndNotify( MessageBase * pCooked );

            /**
            * @brief The cookedWaitAndGet Operation
            *
            * This operation is invoked to obtain cooked memory from the implementation.
            *
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the "cooked" ring buffer becomes aborted.
            *
            * @return Returns a pointer to an abstract MessageBase object from the implementation.
            */
            MessageBase * cookedWaitAndGet();

            /**
            * @brief The rawPutAndNotify Operation
            *
            * This operation restores raw memory back into the our implementation and notifies any
            * waiters for such availability.
            *
            * @param pRaw A pointer to raw memory that is to be made available for cooking.
            *
            * @throw Throws ReiserRT::Core::SemaphoreAborted if the "raw" ring buffer becomes aborted.
            */
            void rawPutAndNotify( void * pRaw );

            /**
            * @brief Obtain a AutoDispatchLock Object
            *
            * Returns an AutoDispatchLock object. The dispatch lock is taken on construction of AutoDispatchLock.
            * When the AutoDispatchLock object is destroy, the dispatch lock is released.
            *
            * @throw Throws ReiserRT::Core::SemaphoreAborted if dispatch locking was not enabled during MessageQueue construction.
            * You must enable dispatch locking during construction of MessageQueue to use this feature.
            *
            * @return Returns an instance of AutoDispatchLock.
            */
            AutoDispatchLock getAutoDispatchLock();

            /**
            * @brief Dispatch a Message
            *
            * This wrapper operation stores the name of the message prior to dispatch. We do this in the case
            * that should the operation throw, the name that threw is recorded.
            * If dispatch locking has been enabled, we will lock the dispatch mutex just prior to dispatching the message.
            *
            * @param pMsg A pointer to the abstract MessageBase being dispatched.
            */
            void dispatchMessage( MessageBase * pMsg );

            /**
            * @brief Get the Name of the Last Message Dispatched.
            *
            * This is more of a diagnostic routine which may be helpful if one particular message type is bombing on dispatch.
            * In order to be useful, the MessageBase::name abstract should be overridden to provide something other than the default
            * implementation. Please see MessageBase::name default implementation for an example of how to implement an override.
            *
            * @return Returns the name of the last message dispatched.
            */
            const char * getNameOfLastMessageDispatched();

            /**
            * @brief The Flush Operation
            *
            * This operation is provided to empty the "cooked" ring buffer and properly destroy any unfetched objects that may remain.
            * Being that we only deal with void pointers at this level, we do not know what type of destructor to invoke.
            * However, through a user provided function object, the destruction can be elevated to the client level, which should know
            * what type of base objects are in the cooked queue.
            *
            * @pre The implementation's "cooked" ring buffer is expected to be in the "Terminal" state to invoke this operation.
            * Violations will result in an exception being thrown.
            *
            * @param operation This is a functor value of type FlushingFunctionType. It must have a valid target
            * (i.e., a nonempty function object). It will be invoked once per remaining object in the "cooked" ring buffer.
            *
            * @throw Throws std::bad_function_call if the operation passed in has no target (an empty function object).
            * @throw Throws ReiserRT::Core::RingBufferStateError if the "cooked" ring buffer is not in the terminal state.
            */
            void flush( FlushingFunctionType operation );

            /**
            * @brief The Is Aborted Operation
            *
            * This operation provides a means to detect whether the abort operation has been preformed.
            *
            * @return Returns true if the abort operation has been invoked.
            */
            bool isAborted() noexcept;

            /**
            * @brief Get the MessageQueueBase Element Size
            *
            * This operation retrieves the fixed size of the Elements managed by MessageQueueBase determined at time of
            * construction. It delegates to the hidden implementation for the information.
            *
            * @return Returns the MessageQueueBase::Imple fixed element size determined at the time of construction.
            */
            size_t getElementSize() noexcept;

        private:
            /**
            * @brief MessageQueue Implementation Reference.
            *
            * A pointer to the hidden implementation.
            */
            Imple * pImple;
        };
    }
}

#endif //REISERRT_CORE_MESSAGEQUEUEBASE_HPP
