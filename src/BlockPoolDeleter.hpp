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
#include <cstdlib>

namespace ReiserRT
{
    namespace Core
    {
        template < typename T >
        class BlockPoolDeleter
        {
        public:
            static_assert( true, "Must use the Array Specialization of BlockPoolDeleter" );
        };

        template < typename T >
        class BlockPoolDeleter< T[] > : public ReiserRT::Core::MemoryPoolDeleterBase
        {
        public:
            explicit BlockPoolDeleter( ReiserRT::Core::MemoryPoolBase * thePool ) noexcept
                    : MemoryPoolDeleterBase{ thePool }
            {
            }

            BlockPoolDeleter() noexcept = default;

            ~BlockPoolDeleter() noexcept = default;

            BlockPoolDeleter( const BlockPoolDeleter & another ) noexcept = default;

            BlockPoolDeleter & operator=( const BlockPoolDeleter & another ) noexcept = default;

            BlockPoolDeleter( BlockPoolDeleter && another ) noexcept = default;

            BlockPoolDeleter & operator =( BlockPoolDeleter && another ) noexcept = default;

            void operator()( T * pT ) noexcept
            {
                if ( !pool ) return;

                if ( !std::is_scalar< T >::value )
                {
                    size_t nElements = getElementSize() / sizeof( T );
                    auto _pT = pT;
                    for ( size_t i = 0; nElements != i; ++i, ++_pT )
                        _pT->~T();
                }

                returnRawBlock( pT );
            }
        };
    }
}

#endif //REISERRT_CORE_BLOCKPOOLDELETER_H
