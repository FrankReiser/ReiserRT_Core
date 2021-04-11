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
        * @brief ObjectPoolElementSizeError Exception Class
        *
        * This class replaced std::runtime_error which CLang-CTidy complained about for being
        * non-nothrow constructible. It is throw  by ObjectPool if "createdObj" is asked to
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
    }
}
#endif //REISERRT_COREEXCEPTIONS_HPP
