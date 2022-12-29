/**
* @file MemoryPoolDeleterBase.cpp
* @brief The Specification file for a Generic Memory Pool Deleter Base Class.
* @authors Frank Reiser
* @date Created on December 27th, 2022
*/

#ifndef REISERRT_CORE_MEMORYPOOLDELETERBASE_H
#define REISERRT_CORE_MEMORYPOOLDELETERBASE_H

#include "ReiserRT_CoreExport.h"

#include <cstdlib>

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief Forward Declaration of Memory Pool Base
        */
        class MemoryPoolBase;

        /**
        * @brief The MemoryPoolDeleterBase Class
        *
        * This class provides the deleter implementation for objects created from MemoryPoolBase which are "owned"
        * by a unique_ptr object. It invokes the object destructor and returns the memory block back to the
        * originating ObjectPool instance.
        */
        class ReiserRT_Core_EXPORT MemoryPoolDeleterBase
        {
        protected:
            /**
            * @brief Qualified Constructor for MemoryPoolDeleterBase
            *
            * Constructs a MemoryPoolDeleterBase object with a pointer reference to the pool base instance which created it.
            *
            * @param thePool A pointer to the object pool instance which invoke the operation.
            */
            explicit inline MemoryPoolDeleterBase( MemoryPoolBase * thePool ) noexcept : pool{ thePool } {}

            /**
            * @brief Default Constructor for MemoryPoolDeleterBase
            *
            * Default construction required for use with empty unique_ptr.
            */
            inline MemoryPoolDeleterBase() noexcept = default;

            /**
            * @brief Default Destructor for MemoryPoolDeleterBase
            *
            * We have no special needs. Therefore we request the default operation which does nothing.
            */
            inline ~MemoryPoolDeleterBase() = default;

            /**
            * @brief Copy Constructor for MemoryPoolDeleterBase
            *
            * This is our copy constructor operation. An implementation must exist to copy MemoryPoolDeleterBase objects
            * as this is what is a minimum requirement for unique_ptr ownership transfer and shared_ptr copying.
            *
            * @param another A reference to an instance being copied.
            */
            MemoryPoolDeleterBase( const MemoryPoolDeleterBase & another ) noexcept = default;

            /**
            * @brief Assignment Operator for MemoryPoolDeleterBase
            *
            * This operation compliments our copy constructor. One shouldn't exist without the other.
            *
            * @param another A reference to an instance being assigned from.
            */
            MemoryPoolDeleterBase & operator=( const MemoryPoolDeleterBase & another ) noexcept = default;

            /**
            * @brief Move Constructor for MemoryPoolDeleterBase
            *
            * Provides for move construction.
            *
            * @param another An rvalue reference to another instance of a MemoryPoolDeleterBase.
            */
            MemoryPoolDeleterBase( MemoryPoolDeleterBase && another ) = default;

            /**
            * @brief Move Assignment Operation for MemoryPoolDeleterBase
            *
            * Provides for move assignment.
            *
            * @param another An rvalue reference to another instance of a MemoryPoolDeleterBase.
            */
            MemoryPoolDeleterBase & operator =( MemoryPoolDeleterBase && another ) = default;

            /**
            * @brief Return Memory to the Pool
            *
            * This protected operation is used solely by a derived deleter object to return memory to the MemoryPoolBase
            * after  an object's destruction. The pointer to this memory is put back into our RingBuffer
            * for subsequent reuse.
            *
            * @param p A pointer to raw memory, that originally came from the MemoryPoolBase.
            */
            void returnRawBlock( void * pV ) noexcept;

            /**
            * @brief Get the Memory Pool Element Size
            *
            * This operation retrieves the size of the elements managed by the memory pool as specified when it
            * was instantiated.
            *
            * @return Returns the MemoryPoolBase::Imple element size specified when the memory pool was instatiated.
            */
            size_t getElementSize() noexcept;

        protected:
            /**
            * @brief A Reference to Our MemoryPoolBase class
            *
            * This attribute records the MemoryPoolBase instance that instantiated the MemoryPoolDeleterBase.
            */
            MemoryPoolBase * pool{ nullptr };
        };

    }
}



#endif //REISERRT_CORE_MEMORYPOOLDELETERBASE_H
