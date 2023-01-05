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
        template < typename T >
        class BlockPool : public ReiserRT::Core::MemoryPoolBase
        {
        public:
            static_assert( std::is_default_constructible< T >::value,
                           "Type T must be no-throw, default constructible!!!" );
            static_assert( std::is_nothrow_destructible< T >::value,
                           "Non-Scalar Type T must be no-throw destructible!!!" );

            using BlockPtrType = BlockPoolPtrType< T >;

            BlockPool() = delete;
            explicit BlockPool( size_t theRequestedNumberOfBlocks, size_t theElementsPerBlock )
                    : ReiserRT::Core::MemoryPoolBase{ theRequestedNumberOfBlocks, sizeof( T ) * theElementsPerBlock }
                    , elementsPerBlock{ theElementsPerBlock }
            {
            }

            BlockPool( const BlockPool & another ) = delete;
            BlockPool & operator =( const BlockPool & another ) = delete;

            BlockPool( BlockPool && another ) = delete;
            BlockPool & operator =( BlockPool && another ) = delete;

            ~BlockPool() = default;

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

                // Wrap in smart pointer, and return via implicit move.
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
            BlockPoolDeleter< T[] > createDeleter() { return std::move( BlockPoolDeleter< T[] >{ this } ); }

            const size_t elementsPerBlock;
        };
    }
}

#endif //REISERRT_CORE_BLOCKPOOL_H
