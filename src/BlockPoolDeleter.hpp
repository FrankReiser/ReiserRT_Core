/**
* @file BlockPoolDeleter.hpp
* @brief The Specification for a Generic BlockPoolDeleter Class.
* @authors Frank Reiser
* @date Created on Jan 5, 2023
*/

#ifndef REISERRT_CORE_BLOCKPOOLDELETER_H
#define REISERRT_CORE_BLOCKPOOLDELETER_H

#include "MemoryPoolDeleterBase.hpp"

#include <type_traits>

namespace ReiserRT
{
    namespace Core
    {
        /**
         * @brief BlockPoolDeleter for Single Element Type
         *
         * BlockPoolDeleter is not intended to be used for single element blocks. ObjectPool
         * and ObjectPoolDeleter deal with single types. This template exists only so that
         * it may be specialized for array types.
         *
         * @note This template cannot be instantiated. Use the specialize version that deals with
         * array types.
         *
         * @tparam T The type of object.
         */
        template < typename T >
        class BlockPoolDeleter
        {
        public:
            static_assert( true, "Must use the Array Specialization of BlockPoolDeleter" );
        };

        /**
        * @brief Template Specialization for BlockPoolDeleter for Array Types
        *
        * This class provides the deleter interface for objects created by BlockPool which are "owned"
        * by a unique_ptr object. Its functor interface invokes the object destructor N number of times
        * and returns the memory block back to the originating BlockPool instance.
        *
        * @tparam T The type of the block array we are dealing with.
        */
        template < typename T >
        class BlockPoolDeleter< T[] > : public ReiserRT::Core::MemoryPoolDeleterBase
        {
        public:
            /**
            * @brief Qualified Constructor for BlockPoolDeleter
            *
            * Constructs a BlockPoolDeleter object with a pointer reference to the pool base instance which created it.
            *
            * @param thePool A pointer to the block pool instance which invoke the operation.
            */
            explicit BlockPoolDeleter( ReiserRT::Core::MemoryPoolBase * thePool ) noexcept
                    : MemoryPoolDeleterBase{ thePool }
            {
            }

            /**
            * @brief Default Constructor for BlockPoolDeleter
            *
            * Default construction required for use with empty unique_ptr.
            */
            BlockPoolDeleter() noexcept = default;

            /**
            * @brief Default Destructor for BlockPoolDeleter
            *
            * We have no special needs. Therefore we request the default operation which does nothing.
            */
            ~BlockPoolDeleter() noexcept = default;

            /**
            * @brief Copy Constructor for BlockPoolDeleter
            *
            * This is our copy constructor operation. An implementation must exist to copy BlockPoolDeleter objects
            * as this is what is a minimum requirement for unique_ptr ownership transfer and shared_ptr copying.
            *
            * @param another A reference to an instance being copied.
            */
            BlockPoolDeleter( const BlockPoolDeleter & another ) noexcept = default;

            /**
            * @brief Assignment Operator for BlockPoolDeleter
            *
            * This operation compliments our copy constructor. One shouldn't exist without the other.
            *
            * @param another A reference to an instance being assigned from.
            */
            BlockPoolDeleter & operator=( const BlockPoolDeleter & another ) noexcept = default;

            /**
            * @brief Move Constructor for BlockPoolDeleter
            *
            * Provides for move construction.
            *
            * @param another An rvalue reference to another instance of a BlockPoolDeleter.
            */
            BlockPoolDeleter( BlockPoolDeleter && another ) noexcept = default;

            /**
            * @brief Move Assignment Operation for BlockPoolDeleter
            *
            * Provides for move assignment.
            *
            * @param another An rvalue reference to another instance of a BlockPoolDeleter.
            */
            BlockPoolDeleter & operator =( BlockPoolDeleter && another ) noexcept = default;

            /**
            * @brief Function Call Interface for BlockPoolDeleter
            *
            * This operation provides the interface necessary for instances of unique_ptr or shared_ptr to
            * destroy the block of objects and return the memory to the pool.
            *
            * @param pT A pointer to the object to be destroyed and returned to the pool.
            * @warning Do not call with a nullptr. Type unique_ptr does not do so when it is destroyed.
            */
            void operator()( T * pT ) noexcept
            {
                if ( !pool ) return;

                // If the type T is not a scalar, we will invoke the destructor for each instance
                // in the block.
                if ( !std::is_scalar< T >::value )
                {
                    size_t nElements = getElementSize() / sizeof( T );
                    auto _pT = pT;
                    for ( size_t i = 0; nElements != i; ++i, ++_pT )
                        _pT->~T();
                }

                returnRawBlock( pT );
            }

            /**
            * @brief Get the Number of Array Elements Managed Within BlockPool Pointer Type.
            *
            *  This operation returns the number of array elements managed within a BlockPool pointer type.
            *  This overcomes a limitation of the array specialization of `std::unique_ptr< T[] >`
            *  where the number of elements of the array cannot be determined via the std`::unique_ptr< T[] >`
            *  itself. Being that we associate this custom deleter type, `std::unique_ptr< T[], D >`,
            *  A reference to D can be obtained through the `get_deleter` operator and from there,
            *  you can call this operation to determine the number of elements in the array.
            *
            * @return Returns the number of array elements managed within a BlockPool pointer type
            */
            [[nodiscard]] size_t getNumElements() const noexcept
            {
                return ( !pool ? 0 : getElementSize() / sizeof( T ) );
            }
        };
    }
}

#endif //REISERRT_CORE_BLOCKPOOLDELETER_H
