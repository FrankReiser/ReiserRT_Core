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
        class ReiserRT_Core_EXPORT MessageQueueBase
        {
        private:
            class Imple;

        protected:
            using CounterType = uint32_t;

            struct RunningStateStats
            {
                RunningStateStats() noexcept = default;

                size_t size{ 0 };               //!< The Size Requested at Construction.
                CounterType runningCount{ 0 };  //!< The Current Running Count Captured Atomically (snapshot)
                CounterType highWatermark{ 0 }; //!< The Current High Watermark Captured Atomically (snapshot)
            };

            using FlushingFunctionType = std::function< void( void * ) noexcept >;

            struct RawMemoryManager {
                RawMemoryManager( MessageQueueBase * pMQBase, void * pTheRaw ) : pMQB{ pMQBase }, pR{ pTheRaw } {}
                ~RawMemoryManager() { if ( pMQB && pR ) pMQB->rawPutAndNotify( pR ); }

                void release() { pR = nullptr; }
            private:
                MessageQueueBase * pMQB;
                void * pR;
            };

            explicit MessageQueueBase( std::size_t requestedNumElements, std::size_t elementSize );
            ~MessageQueueBase();

            RunningStateStats getRunningStateStatistics() noexcept;

            void abort();

            void * rawWaitAndGet();

            void cookedPutAndNotify( void * pCooked );

            void * cookedWaitAndGet();

            void rawPutAndNotify( void * pRaw );

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
