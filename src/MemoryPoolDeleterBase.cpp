/**
* @file MemoryPoolDeleterBase.cpp
* @brief The Implementation file for a Generic Memory Pool Deleter Base Class.
* @authors Frank Reiser
* @date Created on December 27th, 2022
*/

#include "MemoryPoolDeleterBase.hpp"

#include "MemoryPoolBase.hpp"

using namespace ReiserRT;
using namespace ReiserRT::Core;

void MemoryPoolDeleterBase::returnRawBlock( void * pV ) noexcept
{
    // Return the memory to the pool
    pool->returnRawBlock( pV );
}

size_t MemoryPoolDeleterBase::getElementSize() const noexcept
{
    return pool->getElementSize();
}
