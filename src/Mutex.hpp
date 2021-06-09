/**
* @file Mutex.hpp
* @brief The Specification for CoreMutex Utility
* @authors: Frank Reiser
* @date Created on Jul 17, 2017
*/

#ifndef REISERRT_MUTEX_HPP
#define REISERRT_MUTEX_HPP

#include <mutex>

#include "ReiserRT_CoreExport.h"

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief A Custom Mutex Class
        *
        * This class exists primarily to overcome an issue that affects real time performance of C++11
        * processes. With the ratification of the C++11 standard, the C++ language gained portable, native thread support.
        * However, in order to be portable and to not allow "The perfect to be the enemy of good enough",
        * a common set of threading features were provided. In cases where a C++11 user might require additional
        * functionality, "native_handle()" operations were sprinkled around the various interfaces so
        * users could modify a threading object post construction using the "native APIs" available on the
        * target platform.
        *
        * Where it comes to mutexes and the actual type being employed is a POSIX pthread_mutex_t,
        * we have a problem with applications running under real time constraints. Specifically, we want to
        * avoid a phenomena known as priority inversion. The native_handle interface of a mutex object
        * cannot be used to modify the contained pthread_mutex_t object post construction. You must specify
        * the pthread_mutex_t policy attributes at time of pthread_mutex_t construction and these attributes are copied
        * into the mutex. These policy attributes are immutable once the mutex is constructed. This class provides
        * the necessary "Duck Typing" in order to be utilized as a direct std::mutex replacement with lock_guard and
        * unique_lock. However, to utilize C++11 condition variables with this type of mutex,
        * the less efficient std::condition_variable_any must be used. A raw POSIX availability can bypass that
        * overhead.
        */
        ///@todo We are not using custom exception types within Mutex.
        ///@todo Clean up documentation regarding POSIX in member operations.
        ///@todo The non-POSIX fallback implementation has NOT BEEN TESTED!
        class ReiserRT_Core_EXPORT Mutex
        {
        public:
            /**
            * @brief Alias for Native Type
            *
            * This is simply an alias for our native type.
            */
            using NativeHandleType = std::mutex::native_handle_type;

        public:
            /**
            * @brief Qualified Constructor for Mutex
            *
            * This operation constructs a Mutex with the PTHREAD_PRIO_INHERIT mutex protocol.
            */
            Mutex();

            /**
            * @brief Destructor for the Mutex
            *
            * This destructor destroys the encapsulated pthread_mutex_t native mutex instance.
            * Any waiters on a destroyed mutex will almost certainly experience system_error exceptions.
            */
            ~Mutex();

            /**
            * @brief Copy Constructor for Mutex
            *
            * Copying Mutex is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a Mutex.
            */
            Mutex( const Mutex & another ) = delete;

            /**
            * @brief Copy Assignment Operation for Mutex
            *
            * Copying Mutex is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a Mutex.
            */
            Mutex & operator=( const Mutex & another ) = delete;

            /**
            * @brief Move Constructor for Mutex
            *
            * Moving Mutex is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a Mutex.
            */
            Mutex( Mutex && another ) = delete;

            /**
            * @brief Move Assignment Operation for Mutex
            *
            * Moving Mutex is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a Mutex.
            */
            Mutex & operator=( Mutex && another ) = delete;

            /**
            * @brief The Lock Operation
            *
            * This "Duck Type" operation blocks until the native mutex object can be locked (taken).
            *
            * @throw Throws std::system_error should an failure occur attempting to take the lock.
            */
            void lock();

            /**
            * @brief The Try Lock Operation
            *
            * This "Duck Type" operation will lock (take) the native mutex if it is not already locked by another thread.
            *
            * @throw Throws std::system_error should an failure occur attempting to "try" taking the lock.
            *
            * @return Returns true if the mutex was successfully locked and false otherwise.
            */
            bool try_lock();

            /**
            * @brief The Lock Operation
            *
            * This "Duck Type" operation unlocks (gives) the native mutex object. It is intended to be called
            * the the same thread owning a lock on the mutex.
            *
            * @throw Throws std::system_error should an failure occur attempting to unlock the mutex.
            */
            void unlock();

            /**
            * @brief The Native Handle Operation
            *
            * This "Duck Type" operation will return the address of our native handle type.
            *
            * @return Returns the address of our encapsulated native mutex object.
            */
            inline NativeHandleType native_handle() { return nativeHandle; }

        private:
// If we do not have pthreads at our disposal, we will fall back onto a default C++11 std::mutex usage
#ifndef REISER_RT_HAS_PTHREADS
            /**
            * @brief The Mutex
            *
            * This is the non-pthreads default C++11 std::mutex wrapped by this class
            */
            std::mutex stdMutex;
#endif

            /**
            * @brief The Native Type
            *
            * This is our native mutex object type.
            */
            NativeHandleType nativeHandle;
        };
    }
}

#endif /* REISERRT_MUTEX_HPP */
