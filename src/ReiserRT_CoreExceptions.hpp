/**
* @file ReiserRT_CoreExceptions.hpp
* @brief The Specification file for Exceptions Thrown by ReiserRT::Core components
*
* This file came into existence in a effort to eliminated a vulnerability with throwing standard exceptions.
* Most standard exceptions are not not-throw copy constructible.
*
* @authors Frank Reiser
* @date Created on Apr 9, 2021
*/

#ifndef REISERRT_COREEXCEPTIONS_HPP
#define REISERRT_COREEXCEPTIONS_HPP

#include "ReiserRT_CoreExport.h"

#include <stdexcept>

namespace ReiserRT {
    namespace Core {

        /**
        * @brief RingBufferUnderflow Exception Class
        *
        * This class replaced std::underflow_error which CLang-CTidy complained about for being
        * non-nothrow constructible. It is thrown by RingBufferSimple on a "get" invocation while
        * empty.
        */
        class ReiserRT_Core_EXPORT RingBufferUnderflow : public std::runtime_error
        {
        public:
            /**
            * @brief Constructor for RingBufferUnderflow
            *
            * @param msg The message to be delivered by the base class' what member function.
            */
            explicit RingBufferUnderflow( const char * msg ) : std::runtime_error{ msg } {}
        };

        /**
        * @brief RingBufferOverflow Exception Class
        *
        * This class replaced std::overflow_error which CLang-CTidy complained about for being
        * non-nothrow constructible.  It is thrown by RingBufferSimple on a "put" invocation while
        * full.
        */
        class ReiserRT_Core_EXPORT RingBufferOverflow : public std::runtime_error
        {
        public:
            /**
            * @brief Constructor for RingBufferOverflow
            *
            * @param msg The message to be delivered by the base class' what member function.
            */
            explicit RingBufferOverflow( const char * msg ) : std::runtime_error{ msg } {}
        };

        /**
        * @brief RingBufferStateError Exception Class
        *
        * This class replaced std::logic_error which CLang-CTidy complained about for being
        * non-nothrow constructible. It is thrown by RingBufferGuarded by several operations
        * if the RingBufferGuarded instance has not been properly initialized or has been finalized.
        */
        class ReiserRT_Core_EXPORT RingBufferStateError : public std::runtime_error
        {
        public:
            /**
            * @brief Constructor for RingBufferStateError
            *
            * @param msg The message to be delivered by the base class' what member function.
            */
            explicit RingBufferStateError( const char * msg ) : std::runtime_error{ msg } {}
        };

        /**
        * @brief SemaphoreAborted Exception Class
        *
        * This class replaced std::runtime_error which CLang-CTidy complained about for being
        * non-nothrow constructible. It is thrown by Semaphore on several operations if the abort
        * call has ever been invoked.
        */
        class ReiserRT_Core_EXPORT SemaphoreAborted : public std::runtime_error
        {
        public:
            /**
            * @brief Constructor for SemaphoreAborted
            *
            * @param msg The message to be delivered by the base class' what member function.
            */
            explicit SemaphoreAborted( const char * msg ) : std::runtime_error{ msg } {}
        };

        /**
        * @brief SemaphoreOverflow Exception Class
        *
        * This class is thrown if a Semaphore has been given more that the maximum allowable
        * limit of std::numeric_limits< uint32_t >::max() which is roughly 4 billion (2^32-1).
        * It can only really be experienced on an unbounded Semaphore. One in which a maximumAvailableCount
        * has not been specified.
        */
        class ReiserRT_Core_EXPORT SemaphoreOverflow : public std::runtime_error
        {
        public:
            /**
            * @brief Constructor for SemaphoreOverflow
            *
            * @param msg The message to be delivered by the base class' what member function.
            */
            explicit SemaphoreOverflow( const char * msg ) : std::runtime_error{ msg } {}
        };

        /**
        * @brief ObjectPoolElementSizeError Exception Class
        *
        * This class replaced std::runtime_error which CLang-CTidy complained about for being
        * non-nothrow constructible. It is thrown by ObjectPool if "createdObj" is asked to
        * create a derived object that exceeds the size allocated for the elements managed by
        * ObjectPool.
        */
        class ReiserRT_Core_EXPORT ObjectPoolElementSizeError : public std::runtime_error
        {
        public:
            /**
            * @brief Constructor for ObjectPoolElementSizeError
            *
            * @param msg The message to be delivered by the base class' what member function.
            */
            explicit ObjectPoolElementSizeError( const char * msg ) : std::runtime_error{ msg } {}
        };

        /**
        * @brief MessageQueueElementSizeError Exception Class
        *
        * This class replaced std::runtime_error which CLang-CTidy complained about for being
        * non-nothrow constructible. It is thrown by MessageQueue if "put" or "emplace" are asked to
        * create a derived message object that exceeds the size allocated for the elements managed by
        * MessageQueue.
        */
        class ReiserRT_Core_EXPORT MessageQueueElementSizeError : public std::runtime_error
        {
        public:
            /**
            * @brief Constructor for MessageQueueElementSizeError
            *
            * @param msg The message to be delivered by the base class' what member function.
            */
            explicit MessageQueueElementSizeError( const char * msg ) : std::runtime_error{ msg } {}
        };

        /**
        * @brief MessageQueueDispatchLockingDisabled Exception Class
        *
        * This class replaced std::runtime_error which CLang-CTidy complained about for being
        * non-nothrow constructible. It is thrown by MessageQueue if "getAutoDispatchLock" is invoked
        * when dispatch locking has not been explicitly enabled during construction.
        */
        class ReiserRT_Core_EXPORT MessageQueueDispatchLockingDisabled : public std::runtime_error
        {
            public:
                /**
                * @brief Constructor for MessageQueueDispatchLockingDisabled
                *
                * @param msg The message to be delivered by the base class' what member function.
                */
                explicit MessageQueueDispatchLockingDisabled( const char * msg ) : std::runtime_error{ msg } {}
        };
    }
}
#endif //REISERRT_COREEXCEPTIONS_HPP
