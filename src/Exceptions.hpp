/**
* @file Exceptions.hpp
* @brief The Specification file for Exceptions Thrown by ReiserRT::Core components
*
* This file came into existence in a effort to eliminated a vulnerability with throwing standard exceptions.
*
* @authors Frank Reiser
* @date Created on Apr 9, 2021
*/

#ifndef REISERRT_CORE_EXCEPTIONS_HPP
#define REISERRT_CORE_EXCEPTIONS_HPP

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

    }
}
#endif //REISERRT_CORE_EXCEPTIONS_HPP
