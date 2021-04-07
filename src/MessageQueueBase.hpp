/**
* @file MessageQueueBase.hpp
* @brief The Specification file for MessageQueueBase
*
* This file came into existence in a effort to replace and ObjectPool usage and ObjectQueue implementation
* in an effort to eliminate an extra Mutex take/release on enqueueing and de-queueing of messages.
* ObjectQueue will be eliminated in this process.
*
* @authors Frank Reiser
* @date Created on Apr 6, 2015
*/

#include "ReiserRT_CoreExport.h"

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

        class ReiserRT_Core_EXPORT MessageQueueBase
        {
        private:
            class Imple;

        public:
            using WakeupCallFunctionType = std::function<void()>;

            using CounterType = uint32_t;

            struct RunningStateStats
            {
                RunningStateStats() noexcept = default;

                size_t size{ 0 };               //!< The Size Requested at Construction.
                CounterType runningCount{ 0 };  //!< The Current Running Count Captured Atomically (snapshot)
                CounterType highWatermark{ 0 }; //!< The Current High Watermark Captured Atomically (snapshot)
            };

            class AutoDispatchLock
            {
            private:
                friend class MessageQueueBase;

                explicit AutoDispatchLock( MessageQueueBase * pTheMQB );

            public:
                AutoDispatchLock() = delete;

                ~AutoDispatchLock();

                AutoDispatchLock( const AutoDispatchLock & another ) = delete;

                AutoDispatchLock & operator =( const AutoDispatchLock & another ) = delete;

                AutoDispatchLock( AutoDispatchLock && another ) noexcept
                        : pMQB{ another.pMQB }
                {
                    another.pMQB = nullptr;
                }

                AutoDispatchLock & operator =( AutoDispatchLock && another ) noexcept
                {
                    if (this != &another)
                    {
                        pMQB = another.pMQB;
                        another.pMQB = nullptr;
                    }
                    return *this;
                }

            private:
                MessageQueueBase * pMQB;
            };


        protected:
            using FlushingFunctionType = std::function< void( void * ) noexcept >;

            struct RawMemoryManager {
                RawMemoryManager( MessageQueueBase * pMQBase, void * pTheRaw ) : pMQB{ pMQBase }, pR{ pTheRaw } {}
                ~RawMemoryManager() { if ( pMQB && pR ) pMQB->rawPutAndNotify( pR ); }

                void release() { pR = nullptr; }
            private:
                MessageQueueBase * pMQB;
                void * pR;
            };

            struct CookedMemoryManager {
                CookedMemoryManager( MessageQueueBase * pMQBase, MessageBase * pMBase ) : pMQB{ pMQBase }, pMsg{ pMBase } {}
                ~CookedMemoryManager() { if ( pMQB && pMsg ) { pMsg->~MessageBase(); pMQB->rawPutAndNotify( pMsg ); } }

            private:
                MessageQueueBase * pMQB;
                MessageBase * pMsg;
            };

            explicit MessageQueueBase( std::size_t requestedNumElements, std::size_t elementSize,
                                       bool enableDispatchLocking );
            ~MessageQueueBase();

            RunningStateStats getRunningStateStatistics() noexcept;

            void abort();

            void * rawWaitAndGet();

            void cookedPutAndNotify( void * pCooked );

            void * cookedWaitAndGet();

            void rawPutAndNotify( void * pRaw );

            AutoDispatchLock getAutoDispatchLock();

            void dispatchMessage( MessageBase * pMsg );

            const char * getNameOfLastMessageDispatched();

            void flush( FlushingFunctionType operation );

            bool isAborted() noexcept;

        private:
            Imple * pImple;
        };
    }
}

#ifndef REISERRT_CORE_MESSAGEQUEUEBASE_HPP
#define REISERRT_CORE_MESSAGEQUEUEBASE_HPP

#endif //REISERRT_CORE_MESSAGEQUEUEBASE_HPP
