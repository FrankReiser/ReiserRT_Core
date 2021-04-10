/**
* @file ReiserRT_CoreExceptions.hpp
* @brief The Specification file for Exceptions Thrown by ReiserRT::Core components
*
* This file came into existence in a effort to eliminated a vulnerability with throwing standard exceptions.
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

        class ReiserRT_Core_EXPORT RingBufferUnderflow : public std::runtime_error
        {
        public:
            explicit RingBufferUnderflow( const char * msg ) : std::runtime_error{ msg } {}
        };

        class ReiserRT_Core_EXPORT RingBufferOverflow : public std::runtime_error
        {
        public:
            explicit RingBufferOverflow( const char * msg ) : std::runtime_error{ msg } {}
        };

        class ReiserRT_Core_EXPORT RingBufferStateError : public std::runtime_error
        {
        public:
            explicit RingBufferStateError( const char * msg ) : std::runtime_error{ msg } {}
        };

        class ReiserRT_Core_EXPORT SemaphoreAborted : public std::runtime_error
        {
        public:
            explicit SemaphoreAborted( const char * msg ) : std::runtime_error{ msg } {}
        };

    }
}
#endif //REISERRT_COREEXCEPTIONS_HPP
