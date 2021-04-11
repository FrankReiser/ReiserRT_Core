/**
* @file ObjectPoolBase.hpp
* @brief The Specification for a Generic Object Pool Base.
* @authors Frank Reiser
* @date Created on Apr 9, 2015
*/

#ifndef REISERRT_CORE_OBJECTPOOLBASE_HPP
#define REISERRT_CORE_OBJECTPOOLBASE_HPP

#include "ReiserRT_CoreExport.h"

#include "ObjectPoolDeleter.hpp"

#include <memory>
#include <type_traits>
#include <functional>
#include <string>
#include <stdexcept>

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief The ObjectPoolBase Class
        *
        * This class provides a base for all specialized ObjectPool template instantiations.
        * It provides a hidden implementation and the primary interface operations required for ObjectPool.
        */
        class ReiserRT_Core_EXPORT ObjectPoolBase
        {
        private:
            /**
            * @brief Forward Declaration of Imple
            *
            * This is our forward declaration of our Hidden Implementation.
            */
            class Imple;

            /**
            * @brief Performance Tracking Feature Counter Type
            *
            * The ObjectPoolBase can keep track of certain performance characteristics. These would primarily be a
            * "Low Watermark" and the current "Running Count". Since this is a pool, the "Running Count" starts
            * out full (high) and so would the "Low Watermark". We use a 32 bit value for these, which should be
            * adequate. On 20200707, the internal ring buffer is maximum is limited to 1 Mega blocks.
            */
            using CounterType = uint32_t;

            /**
            * @brief Friend Class Declaration.
            *
            * ObjectPoolDeleterBase requires access to our returnRawBlock operation and we wish no other access
            * other than that of derived types of ObjectPool (template instantiations).
            */
            friend class ObjectPoolDeleterBase;

        public:
            /**
            * @brief A Return Type for Inquiring Clients
            *
            * This structure provides the essential information for clients inquiring on performance
            * measurements. It represents a snapshot of the current state of affairs at invocation.
            * The "Low Watermark" is significantly more stable than the "Running Count" which may be erratic,
            * dependent on the client.
            */
            struct RunningStateStats
            {
                /**
                * @brief Default Constructor for RunningStateStats
                *
                * This operation uses the compiler generated default, which in our case is to default the data
                * members to zero.
                */
                RunningStateStats() noexcept = default;

                size_t size{ 0 };               //!< The Size of the Pool Requested Rounded Up to Next power of Two.
                CounterType runningCount{ 0 };  //!< The Current Running Count Captured Atomically (snapshot)
                CounterType lowWatermark{ 0 };  //!< The Current Low Watermark Captured Atomically (snapshot)
            };

        protected:
            /**
            * @brief Default Constructor for ObjectPoolBase
            *
            * Default construction of ObjectPoolBase is disallowed. Hence, this operation has been deleted.
            */
            ObjectPoolBase() = delete;

            /**
            * @brief Qualified Constructor for ObjectPool
            *
            * This qualified constructor builds an ObjectPool using the requestedNumElements and element size argument
            * values. It delegates to the hidden implementation to fill the construction requirements.
            *
            * @param requestedNumElements The requested ObjectPool size. This will be rounded up to the next whole
            * power of two and clamped within RingBuffer design limits.
            * @param elementSize The maximum size of each allocation block.
            */
            explicit ObjectPoolBase( size_t requestedNumElements, size_t elementSize );

            /**
            * @brief Copy Constructor for ObjectPoolBase
            *
            * Copying ObjectPoolBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a ObjectPoolBase.
            */
            ObjectPoolBase( const ObjectPoolBase & another ) = delete;

            /**
            * @brief Copy Assignment Operation for ObjectPoolBase
            *
            * Copying ObjectPoolBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a ObjectPoolBase of the same template type.
            */
            ObjectPoolBase & operator =( const ObjectPoolBase & another ) = delete;

            /**
            * @brief Move Constructor for ObjectPoolBase
            *
            * Moving ObjectPoolBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a ObjectPoolBase of the same template type.
            */
            ObjectPoolBase( ObjectPoolBase && another ) = delete;

            /**
            * @brief Move Assignment Operation for ObjectPoolBase
            *
            * Moving ObjectPoolBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a ObjectPoolBase of the same template type.
            */
            ObjectPoolBase & operator =( ObjectPoolBase && another ) = delete;

            /**
            * @brief Destructor for ObjectPoolBase
            *
            * The destructor destroys the hidden implementation object.
            */
            ~ObjectPoolBase();

            /**
            * @brief The Get Raw Block Operation
            *
            * This operation requests a block of memory from the pool.
            *
            * @throw Throws ReiserRT::Core::RingBufferOverflow if the memory pool has been exhausted.
            * @returns A pointer to the raw memory block.
            */
            void * getRawBlock();

            /**
            * @brief The Return Raw Block Operation
            *
            * This operation returns a block of memory to the pool for subsequent reuse.
            *
            * @param pRaw A pointer to the raw block of memory to return to the pool.
            */
            void returnRawBlock( void * pRaw ) noexcept;

            /**
            * @brief Create a Concrete ObjectPoolDeleter Object
            *
            * This operation creates an ObjectPoolDeleter locally and moves it off the stack for return.
            * No heap usage is required.
            *
            * @tparam The type of object to which a ObjectPoolDeleter instance is required.
            * @return An instance of a concrete ObjectPoolDeleter object moved off the stack.
            */
            template < typename T >
            ObjectPoolDeleter< T > createDeleter() { return std::move(ObjectPoolDeleter< T >{this} ); }

            /**
            * @brief Get the ObjectPoolBase Size
            *
            * This operation retrieves the fixed size of the ObjectPoolBase determined at the time of construction.
            * It delegates to the hidden implementation for the information.
            *
            * @return Returns the ObjectPoolBase::Imple fixed size determined at the time of construction.
            */
            size_t getSize() noexcept;

            /**
            * @brief Get the ObjectPoolBase Element Size
            *
            * This operation retrieves the fixed size of the Elements managed by ObjectPoolBase determined at time of
            * construction. It delegates to the hidden implementation for the information.
            *
            * @return Returns the ObjectPoolBase::Imple fixed element size determined at the time of construction.
            */
            size_t getElementSize() noexcept;

            /**
            * @brief Get the Running State Statistics
            *
            * This operation provides for performance monitoring of the ObjectPoolBase. The data returned
            * is an atomically captured snapshot of the RunningStateStats. The "low watermark",
            * compared to the size can provide an indication of the exhaustion level.
            *
            * @return Returns a snapshot of internal RunningStateStats.
            */
            RunningStateStats getRunningStateStatistics() noexcept;

            /**
            * @brief The Hidden Implementation Instance
            *
            * This attribute stores an instance of our hidden implementation.
            */
            Imple * pImple;
        };

    }
}

#endif /* REISERRT_CORE_OBJECTPOOLBASE_HPP */
