/**
* @file CoreMutex.cpp
* @brief The Specification for CoreMutex Utility
* @authors: Frank Reiser
* @date Created on Jul 17, 2017
*/

#include "Mutex.hpp"

#ifdef REISER_RT_HAS_PTHREADS

using namespace ReiserRT::Core;

Mutex::Mutex()
{
    // Initialize a mutex attribute
    pthread_mutexattr_t attr;
    pthread_mutexattr_init( &attr );

    // Set protocol to PTHREAD_PRIO_INHERIT if not already the case.
    int protocol;
    pthread_mutexattr_getprotocol( &attr, &protocol );
    if ( protocol != PTHREAD_PRIO_INHERIT )
    {
        pthread_mutexattr_setprotocol( &attr, PTHREAD_PRIO_INHERIT );
    }

    // Initialize our native type (pthread_mutex_t) with our modified attribute.
    pthread_mutex_init( &nativeType, &attr );

    // Destroy the attribute, we are done with it.
    pthread_mutexattr_destroy( &attr );
}

Mutex::~Mutex()
{
    // Destroy the mutex
    pthread_mutex_destroy( &nativeType );
}

void Mutex::lock()
{
    int e = pthread_mutex_lock( &nativeType );

    // EINVAL, EAGAIN, EBUSY, EINVAL and EDEADLK(maybe)
    if ( e ) throw std::system_error{ e, std::system_category() };
}

bool Mutex::try_lock()
{
    int e = pthread_mutex_trylock( &nativeType );

    // EBUSY means it's already locked and we cannot acquire it. Anything else is an error.
    if ( e != 0 && e != EBUSY ) throw std::system_error{ e, std::system_category() };

    return e == 0;
}

void Mutex::unlock()
{
    int e = pthread_mutex_unlock( &nativeType );

    // EINVAL, EAGAIN and  EPERM potentially.
    if ( e ) throw std::system_error{ e, std::system_category() };
}

#endif


