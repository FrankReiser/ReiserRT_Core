/**
* @file PriorityInheritMutex.h
* @brief The Specification for PriorityInheritMutex Utility
* @authors: Frank Reiser
* @date Created on Jul 17, 2017
*/

#include "PriorityInheritMutex.h"

#ifdef REISER_RT_GCC

using namespace ReiserRT::Core;

PriorityInheritMutex::PriorityInheritMutex()
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

PriorityInheritMutex::~PriorityInheritMutex()
{
    // Destroy the mutex
    pthread_mutex_destroy( &nativeType );
}

#endif


