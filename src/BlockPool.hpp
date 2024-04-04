/**
 * @file BlockPool.h
 * @brief The specification file for the Block Pool
 * @authors Frank Reiser
 * @date Initiated Jan 5, 2023
 */

#ifndef REISERRT_CORE_BLOCKPOOL_H
#define REISERRT_CORE_BLOCKPOOL_H

#include "MemoryPoolBase.hpp"
#include "BlockPoolFwd.hpp"
#include "BlockPoolDeleter.hpp"

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief A Generic Block Pool Implementation with Limited Factory Functionality
        *
        * This template class provides a compile-time, high performance, generic object block factory from a pre-allocated
        * memory pool. Blocks of types created and delivered from this implementation are encapsulated
        * inside of a C++11 unique_ptr with a custom "Deleter" which automatically returns memory to this pool when the
        * unique_ptr is destroyed. Please @see BlockPoolDeleter for details. This was initially designed to handle
        * blocks of scalar types. However, it supports any type that is default constructible.
        *
        * @tparam T The type of object that will be created from the pool. The type must be default constructible
        * and no-throw destructible.
        */
        template < typename T >
        class BlockPool : public ReiserRT::Core::MemoryPoolBase
        {
        public:
            static_assert( std::is_default_constructible< T >::value,
                           "Type T must be no-throw, default constructible!!!" );
            static_assert( std::is_nothrow_destructible< T >::value,
                           "Non-Scalar Type T must be no-throw destructible!!!" );

            /**
            * @brief The Return Value Type for the `getBlock` Operation
            *
            * This type definition describes the unique_ptr specialization that we return on a `getBlock` invocation.
            * The details can be found within "BlockPoolFwd.hpp".
            */
            using BlockPtrType = BlockPoolPtrType< T >;

            /**
            * @brief Default Constructor for BlockPool Disallowed
            *
            * Default construction of BlockPool is disallowed. Hence, this operation has been deleted.
            */
            BlockPool() = delete;

            /**
            * @brief Qualified Constructor for BlockPool
            *
            * This qualified constructor builds a BlockPool using the requestedNumElements
            * and the theElementsPerBlock argument values. It delegates to its base class to satisfy
            * the majority of construction requirements.
            *
            * @param requestedNumberOfBlocks The requested BlockPool size in blocks. This will be rounded up to the next whole
            * power of two and clamped within RingBuffer design limits.
            * @param theElementsPerBlock. This specifies the number of elements to be delivered in a call to `getBlock`.
            */
            explicit BlockPool( size_t requestedNumberOfBlocks, size_t theElementsPerBlock )
              : ReiserRT::Core::MemoryPoolBase{ requestedNumberOfBlocks, sizeof( T ) * theElementsPerBlock }
              , elementsPerBlock{ theElementsPerBlock }
            {
            }

            /**
            * @brief Copy Constructor for BlockPool Disallowed
            *
            * Copying BlockPool is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a ObjectPool of the same templated type.
            */
            BlockPool( const BlockPool & another ) = delete;

            /**
            * @brief Copy Assignment Operation for BlockPool Disallowed
            *
            * Copying ObjectPool is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a ObjectPool of the same templated type.
            */
            BlockPool & operator =( const BlockPool & another ) = delete;

            /**
            * @brief Destructor for BlockPool
            *
            * This destructor does little but delegate to the base class for the required clean-up.
            *
            * @note Destroying an BlockPool while pointers to blocks are allocated and essentially
            * "loaned", would be a terrible thing to do. It will almost certainly lead to an exception
            *  being thrown.
            */
            ~BlockPool() = default;

            /**
            * @brief The Get Block Operation
            *
            * This operation gets a block of object memory from the pool. The number of elements in the block
            * are that which was specified during construction. The objects are default constructed.
            * This is unlike ObjectPool which allows for the creation of single object, derived types
            * and construction parameters.
            * None of this is supported by BlockPool. Only the base type may be constructed and
            * without parameters, implying default construction only.
            *
            * @return This operation returns the newly constructed object block, wrapped within a std::unique_ptr,
            * associated with our custom Deleter. This unique_ptr is aliased as BlockPtrType.
            *
            * @throw Throws ReiserRT::Core::RingBufferOverflow on memory pool exhaustion.
            * @note May throw other exceptions if the block construction of the specified type throws an exception.
            */
            BlockPtrType getBlock()
            {
                void * pRaw = getRawBlock();

                // This would benefit from C++17 "if constexpr" statements to select the
                // right code at compile time. However, it is expected that the compiler will optimize
                // away the branches not available for a particular type
                T * pCooked;
                if ( std::is_scalar< T >::value )
                    pCooked = reinterpret_cast< T * >( pRaw );  // Default scalar values are zero.
                else if ( std::is_nothrow_default_constructible< T >::value )
                    pCooked = new( pRaw )T[ elementsPerBlock ];
                else
                {
                    // Possible throw on construction. We will ensure that no pool memory is leaked should
                    // an exception be thrown. The new operator itself will destroy any objects
                    // that were successfully constructed prior to any exception while constructing the sequence.
                    // If we succeed without throwing, then we release the RawMemoryManager instance from responsibility
                    // of returning raw memory to the pool.
                    RawMemoryManager rawMemoryManager{ this, pRaw };
                    pCooked = new( pRaw )T[ elementsPerBlock ];
                    rawMemoryManager.release();
                }

                // Wrap cooked data in a smart pointer, and return via implicit move.
                return BlockPtrType{ pCooked, std::move( createDeleter() ) };
            }

            /**
            * @brief Get the BlockPool size
            *
            * This declaration brings the base class functionality into the public scope.
            */
            using MemoryPoolBase::getSize;

            /**
            * @brief Get the Running State Statistics
            *
            * This declaration brings the base class functionality into the public scope.
            */
            using MemoryPoolBase::getRunningStateStatistics;

        private:
            /**
            * @brief Create a Concrete BlockPoolDeleter Object
            *
            * This operation creates an BlockPoolDeleter locally and moves it off the stack for return.
            *
            * @return An instance of a concrete BlockPoolDeleter object moved off the stack.
            */
            BlockPoolDeleter< T[] > createDeleter() { return std::move( BlockPoolDeleter< T[] >{ this } ); }

            /**
            * @brief The Number of Elements Per Block
            */
            const size_t elementsPerBlock;
        };
    }
}

#endif //REISERRT_CORE_BLOCKPOOL_H
