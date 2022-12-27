/**
* @file ObjectPoolDeleter.cpp
* @brief The Implementation for a Generic ObjectPoolDeleter Class.
* @authors Frank Reiser
* @date Created on Mar 23, 2021
*/

#include "ObjectPoolDeleter.hpp"
#include "MemoryPoolBase.hpp"

using namespace ReiserRT;
using namespace ReiserRT::Core;

void ReiserRT::Core::ObjectPoolDeleterBase::returnRawBlock(void * pV ) noexcept
{
    // Return the memory to the pool
    pool->returnRawBlock( pV );
}
