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
        /**
        * @brief Forward Declaration of BlockPool Template
        */
        template< typename T >
        class BlockPool;

        /**
        * @brief Forward Declaration of "Non-specialized" BlockPoolDeleter Template.
        *
        * This is required in order to declare the specialized version that deals with arrays.
        */
        template< typename T >
        class BlockPoolDeleter;

        /**
        * @brief Forward Declaration of "Specialized" BlockPoolDeleter Template.
        */
        template< typename T >
        class BlockPoolDeleter<T[]>;

        /**
        * @brief Alias Type for What Block::getBlock Returns.
        */
        template < typename T >
        using BlockPoolPtrType = std::unique_ptr< T[], BlockPoolDeleter< T[] > >;
    }
}

#endif //REISERRT_CORE_BLOCKPOOLFWD_H
