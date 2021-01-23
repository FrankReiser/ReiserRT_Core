/**
* @file PriorityInheritMutex.h
* @brief The Specification for PriorityInheritMutex Utility
* @authors: Frank Reiser
* @date Created on Jul 17, 2017
*/

#ifndef PRIORITYINHERITMUTEX_H_
#define PRIORITYINHERITMUTEX_H_

#include "PlatformDetect.h"

#ifdef REISER_RT_GCC
#include <pthread.h>
#include <system_error>

namespace ReiserRT
{
    namespace Core
    {
        class PriorityInheritMutex
        {
        public:
            using NativeType = pthread_mutex_t;
            using NativeHandleType = NativeType *;

        public:
            PriorityInheritMutex();
            ~PriorityInheritMutex();

            PriorityInheritMutex( const PriorityInheritMutex & another ) = delete;
            PriorityInheritMutex & operator=( const PriorityInheritMutex & another ) = delete;

            PriorityInheritMutex( PriorityInheritMutex && another ) = delete;
            PriorityInheritMutex & operator=( PriorityInheritMutex && another ) = delete;

            inline void lock()
            {
                int e = pthread_mutex_lock( &nativeType );

                // EINVAL, EAGAIN, EBUSY, EINVAL and EDEADLK(maybe)
                if ( e ) throw std::system_error( e, std::system_category() );
            }

            inline bool try_lock()
            {
                int e = pthread_mutex_trylock( &nativeType );

                // EBUSY means it's already locked and we cannot acquire it. Anything else is an error.
                if ( e != 0 && e != EBUSY ) throw std::system_error( e, std::system_category() );

                return e == 0;
            }

            inline void unlock()
            {
                int e = pthread_mutex_unlock( &nativeType );

                // EINVAL, EAGAIN and  EPERM potentially.
                if ( e ) throw std::system_error( e, std::system_category() );
            }

            inline NativeHandleType native_handle() { return &nativeType; }

        private:
            NativeType nativeType;
        };
    }
}

#endif // REISER_RT_GCC

#endif /* PRIORITYINHERITMUTEX_H_ */
