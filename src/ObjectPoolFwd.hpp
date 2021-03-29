/**
* @file ObjectPoolFwd.hpp
* @brief Forward Declarations for using ObjectPool and the pointer type it returns.
* @authors Frank Reiser
* @date Created on Mar 23, 2021
*/

#ifndef REISERRT_CORE_OBJECTPOOLFWD_HPP
#define REISERRT_CORE_OBJECTPOOLFWD_HPP

#include <memory>

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief Forward Declaration of ObjectPool Template
        */
        template< typename T >
        class ObjectPool;

        /**
        * @brief Forward Declaration of ObjectPoolDeleter Template
        */
        template< typename T >
        class ObjectPoolDeleter;

        /**
        * @brief Alias Type for What ObjectPool::createObj Returns.
        */
        template < typename T >
        using ObjectPoolPtrType = std::unique_ptr< T, ObjectPoolDeleter< T > >;
    }
}

#endif //REISERRT_CORE_OBJECTPOOLFWD_HPP
