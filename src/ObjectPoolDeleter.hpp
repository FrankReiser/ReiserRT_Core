/**
* @file ObjectPoolDeleter.hpp
* @brief The Specification for a Generic ObjectPoolDeleter Class.
* @authors Frank Reiser
* @date Created on Mar 23, 2021
*/

#ifndef OBJECTPOOLDELETER_HPP
#define OBJECTPOOLDELETER_HPP

#include "ReiserRT_CoreExport.h"

namespace ReiserRT
{
    namespace Core
    {
        // Forward Declaration
        class ReiserRT_Core_EXPORT ObjectPoolBase;

        /**
        * @brief The ObjectPoolDeleterBase
        *
        * This class provides the deleter implementation for objects created by ObjectPool which are "owned"
        * by a unique_ptr object. It invokes the object destructor and returns the memory block back to the
        * originating ObjectPool instance.
        *
        * This base class is generic as is ObjectPoolBase. It only deals with void pointers. This became necessary
        * because the compiler was somehow optimizing away lambda functions created against the interface of
        * ObjectPoolDeleter. This only became necessary when it was decided to extract the "Deleter" specification from
        * the templated ObjectPool class to simplify the declaration of ObjectPool, ObjectPoolPtrType for easier usage.
        * Having this concrete, non-template base class alleviated the issue.
        */
        class ReiserRT_Core_EXPORT ObjectPoolDeleterBase
        {
        protected:
            /**
            * @brief Qualified Constructor for ObjectPoolDeleterBase
            *
            * Constructs a ObjectPoolDeleterBase object with a pointer reference to the pool base instance which created it.
            *
            * @param thePool A pointer to the object pool instance which invoke the operation.
            */
            explicit inline ObjectPoolDeleterBase( ObjectPoolBase * thePool ) noexcept : pool{ thePool } {}

            /**
            * @brief Default Constructor for ObjectPoolDeleterBase
            *
            * Default construction required for use with empty unique_ptr.
            */
            inline ObjectPoolDeleterBase() noexcept = default;

            /**
            * @brief Default Destructor for ObjectPoolDeleterBase
            *
            * We have no special needs. Therefore we request the default operation which does nothing.
            */
            inline ~ObjectPoolDeleterBase() = default;

            /**
            * @brief Copy Constructor for ObjectPoolDeleterBase
            *
            * This is our copy constructor operation. An implementation must exist to copy ObjectPoolDeleterBase objects
            * as this is what is a minimum requirement for unique_ptr ownership transfer and shared_ptr copying.
            *
            * @param another A reference to an instance being copied.
            */
            ObjectPoolDeleterBase( const ObjectPoolDeleterBase & another ) noexcept = default;

            /**
            * @brief Assignment Operator for ObjectPoolDeleterBase
            *
            * This operation compliments our copy constructor. One shouldn't exist without the other.
            *
            * @param another A reference to an instance being assigned from.
            */
            ObjectPoolDeleterBase & operator=( const ObjectPoolDeleterBase & another ) noexcept = default;

            /**
            * @brief Move Constructor for ObjectPoolDeleterBase
            *
            * Provides for move construction.
            *
            * @param another An rvalue reference to another instance of a ObjectPoolDeleterBase.
            */
            ObjectPoolDeleterBase( ObjectPoolDeleterBase && another ) = default;

            /**
            * @brief Move Assignment Operation for ObjectPoolDeleterBase
            *
            * Provides for move assignment.
            *
            * @param another An rvalue reference to another instance of a ObjectPoolDeleterBase.
            */
            ObjectPoolDeleterBase & operator =( ObjectPoolDeleterBase && another ) = default;

            /**
            * @brief Return Memory to the Pool
            *
            * This protected operation is used solely by our Deleter object to return memory to the ObjectPool after
            * an object's destruction. The pointer to this memory is put back into our RingBuffer for subsequent reuse.
            *
            * @param p A pointer to raw memory, that originally came from the ObjectPool.
            */
            void returnRawBlock( void * pV ) noexcept;

        protected:
            /**
            * @brief A Reference to Our Object Pool Base class
            *
            * This attribute records the ObjectPoolBase instance that instantiated the ObjectPoolDeleterBase.
            */
            ObjectPoolBase * pool{ nullptr };
        };

        /**
        * @brief The ObjectPoolDeleter
        *
        * This class provides the deleter interface for objects created by ObjectPool which are "owned"
        * by a unique_ptr object. It invokes the object destructor and returns the memory block back to the
        * originating ObjectPool instance.
        */
        template < typename T >
        class ReiserRT_Core_EXPORT ObjectPoolDeleter : public ObjectPoolDeleterBase
        {
        private:
            /**
            * @brief Friend Declaration
            *
            * We declare ObjectPoolBase as a friend as we only allow it to construct us.
            */
            friend class ObjectPoolBase;

            /**
            * @brief Qualified Constructor for ObjectPoolDeleter
            *
            * Constructs a ObjectPoolDeleter object with a pointer reference to the pool base instance which created it.
            *
            * @param thePool A pointer to the object pool instance which invoke the operation.
            */
            explicit ObjectPoolDeleter( ObjectPoolBase * thePool ) noexcept : ObjectPoolDeleterBase{ thePool } {}

        public:
            /**
            * @brief Default Constructor for ObjectPoolDeleter
            *
            * Default construction required for use with empty unique_ptr.
            */
            ObjectPoolDeleter() noexcept = default;

            /**
            * @brief Default Destructor for ObjectPoolDeleter
            *
            * We have no special needs. Therefore we request the default operation which does nothing.
            */
            ~ObjectPoolDeleter() = default;

            /**
            * @brief Copy Constructor for ObjectPoolDeleter
            *
            * This is our copy constructor operation. An implementation must exist to copy ObjectPoolDeleter objects
            * as this is what is a minimum requirement for unique_ptr ownership transfer and shared_ptr copying.
            *
            * @param another A reference to an instance being copied.
            */
            ObjectPoolDeleter( const ObjectPoolDeleter & another ) noexcept = default;

            /**
            * @brief Assignment Operator for ObjectPoolDeleter
            *
            * This operation compliments our copy constructor. One shouldn't exist without the other.
            *
            * @param another A reference to an instance being assigned from.
            */
            ObjectPoolDeleter & operator=( const ObjectPoolDeleter & another ) noexcept = default;

            /**
            * @brief Move Constructor for ObjectPoolDeleter
            *
            * Provides for move construction.
            *
            * @param another An rvalue reference to another instance of a ObjectPoolDeleter.
            */
            ObjectPoolDeleter( ObjectPoolDeleter && another ) noexcept = default;

            /**
            * @brief Move Assignment Operation for ObjectPoolDeleter
            *
            * Provides for move assignment.
            *
            * @param another An rvalue reference to another instance of a ObjectPoolDeleter.
            */
            ObjectPoolDeleter & operator =( ObjectPoolDeleter && another ) noexcept = default;

            /**
            * @brief Function Call Interface for ObjectPoolDeleter
            *
            * This operation provides the interface necessary for instances of unique_ptr or shared_ptr to
            * destroy the object and return the memory to the pool.
            *
            * @param pT A pointer to the object to be destroyed and returned to the pool.
            * @warning Do not call with a nullptr. Type unique_ptr does not do so when it is destroyed.
            */
            void operator()( T * pT ) noexcept { pT->~T(); if ( pool ) returnRawBlock( pT ); }
        };
    }
}

#endif //OBJECTPOOLDELETER_HPP
