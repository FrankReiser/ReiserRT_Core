/**
* @file ObjectPoolDeleter.hpp
* @brief The Specification for a Generic ObjectPoolDeleter Class.
* @authors Frank Reiser
* @date Created on Mar 23, 2021
*/

#ifndef REISERRT_CORE_OBJECTPOOLDELETER_HPP
#define REISERRT_CORE_OBJECTPOOLDELETER_HPP

#include "MemoryPoolDeleterBase.hpp"

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief The ObjectPoolDeleter
        *
        * This class provides the deleter interface for objects created by ObjectPool which are "owned"
        * by a unique_ptr object. It invokes the object destructor and returns the memory block back to the
        * originating ObjectPool instance.
        */
        template < typename T >
        class ObjectPoolDeleter : public MemoryPoolDeleterBase
        {
        public:
            /**
            * @brief Qualified Constructor for ObjectPoolDeleter
            *
            * Constructs a ObjectPoolDeleter object with a pointer reference to the pool base instance which created it.
            *
            * @param thePool A pointer to the object pool instance which invoke the operation.
            */
            explicit ObjectPoolDeleter( MemoryPoolBase * thePool ) noexcept : MemoryPoolDeleterBase{ thePool } {}

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
            ~ObjectPoolDeleter() noexcept = default;

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

#endif //REISERRT_CORE_OBJECTPOOLDELETER_HPP
