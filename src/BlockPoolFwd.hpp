/**
* @file BlockPoolFwd.hpp
* @brief Forward Declarations for using BlockPool and the pointer type it returns.
* @authors Frank Reiser
* @date Created on Jan 5, 2023
*/

#ifndef REISERRT_CORE_BLOCKPOOLFWD_H
#define REISERRT_CORE_BLOCKPOOLFWD_H

#include <memory>

namespace ReiserRT
{
    namespace Core
    {
        template< typename T >
        class BlockPool;

        template< typename T >
        class BlockPoolDeleter;

        template< typename T >
        class BlockPoolDeleter<T[]>;

        template < typename T >
        using BlockPoolPtrType = std::unique_ptr< T[], BlockPoolDeleter< T[] > >;
    }
}

#endif //REISERRT_CORE_BLOCKPOOLFWD_H
