/**
* @file Mutex.cpp
* @brief The Specification for CoreMutex Utility
* @authors: Frank Reiser
* @date Created on Jul 17, 2017
*/

#include "Mutex.hpp"

#include <pthread.h>
#include <system_error>

using namespace ReiserRT::Core;

Mutex::Mutex()
#ifdef REISER_RT_HAS_PTHREADS
  : nativeHandle{new pthread_mutex_t }
#else
  : stdMutex{}
  , nativeHandle{ stdMutex.native_handle() }
#endif
{
#ifdef REISER_RT_HAS_PTHREADS
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
    nativeHandle = new( pthread_mutex_t );
    pthread_mutex_init( nativeHandle, &attr );

    // Destroy the attribute, we are done with it.
    pthread_mutexattr_destroy( &attr );
#endif
}

Mutex::~Mutex()
{
#ifdef REISER_RT_HAS_PTHREADS
    // Destroy the mutex and delete nativeHandle allocated during construction.
    pthread_mutex_destroy( nativeHandle );
    delete nativeHandle;
#endif
}

void Mutex::lock()
{
#ifdef REISER_RT_HAS_PTHREADS
    int e = pthread_mutex_lock( nativeHandle );

    // EINVAL, EAGAIN, EBUSY, EINVAL and EDEADLK(maybe)
    if ( e ) throw std::system_error{ e, std::system_category() };
#else
    stdMutex.lock();
#endif
}

bool Mutex::try_lock()
{
#ifdef REISER_RT_HAS_PTHREADS
    int e = pthread_mutex_trylock( nativeHandle );

    // EBUSY means it's already locked and we cannot acquire it. Anything else is an error.
    if ( e != 0 && e != EBUSY ) throw std::system_error{ e, std::system_category() };

    return e == 0;
#else
    return stdMutex.try_lock();
#endif
}

void Mutex::unlock()
{
#ifdef REISER_RT_HAS_PTHREADS
    int e = pthread_mutex_unlock( nativeHandle );

    // EINVAL, EAGAIN and  EPERM potentially.
    if ( e ) throw std::system_error{ e, std::system_category() };
#else
    stdMutex.unlock();
#endif
}


