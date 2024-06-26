/**
* @file MemoryPoolBase.hpp
* @brief The Specification for a Generic Object Pool Base.
* @authors Frank Reiser
* @date Created on Apr 9, 2015
*/

#ifndef REISERRT_CORE_MEMORYPOOLBASE_HPP
#define REISERRT_CORE_MEMORYPOOLBASE_HPP

#include "ReiserRT_CoreExport.h"

#include <cstdlib>
#include <cstdint>

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief The MemoryPoolBase Class
        *
        * This class provides a basis for other classes that serve as Object or Block memory pools.
        * It provides a hidden implementation and the necessary interface operations for such use cases.
        */
        class ReiserRT_Core_EXPORT MemoryPoolBase
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
            * The MemoryPoolBase can keep track of certain performance characteristics. These would primarily be a
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
            friend class MemoryPoolDeleterBase;

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

            /**
            * @brief Default Constructor for MemoryPoolBase
            *
            * Default construction of MemoryPoolBase is disallowed. Hence, this operation has been deleted.
            */
            MemoryPoolBase() = delete;

        protected:
            /**
            * @brief Qualified Constructor for MemoryPoolBase
            *
            * This qualified constructor builds an ObjectPool using the requestedNumElements and element size argument
            * values. It delegates to the hidden implementation to fill the construction requirements.
            *
            * @param requestedNumElements The requested ObjectPool size. This will be rounded up to the next whole
            * power of two and clamped within RingBuffer design limits.
            * @param elementSize The maximum size of each allocation block.
            */
            explicit MemoryPoolBase( size_t requestedNumElements, size_t elementSize );

        public:
            /**
            * @brief Copy Constructor for MemoryPoolBase
            *
            * Copying MemoryPoolBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a MemoryPoolBase.
            */
            MemoryPoolBase( const MemoryPoolBase & another ) = delete;

            /**
            * @brief Copy Assignment Operation for MemoryPoolBase
            *
            * Copying MemoryPoolBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a MemoryPoolBase of the same template type.
            */
            MemoryPoolBase & operator =( const MemoryPoolBase & another ) = delete;

            /**
            * @brief Move Constructor for MemoryPoolBase
            *
            * Moving MemoryPoolBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a MemoryPoolBase of the same template type.
            */
            MemoryPoolBase( MemoryPoolBase && another ) = delete;

            /**
            * @brief Move Assignment Operation for MemoryPoolBase
            *
            * Moving MemoryPoolBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a MemoryPoolBase of the same template type.
            */
            MemoryPoolBase & operator =( MemoryPoolBase && another ) = delete;

        protected:
            /**
            * @brief Destructor for MemoryPoolBase
            *
            * The destructor destroys the hidden implementation object.
            */
            ~MemoryPoolBase();

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

        public:
            /**
            * @brief Get the MemoryPoolBase Size
            *
            * This operation retrieves the fixed size of the MemoryPoolBase determined at the time of construction.
            * It delegates to the hidden implementation for the information.
            *
            * @return Returns the MemoryPoolBase::Imple fixed size determined at the time of construction.
            */
            [[nodiscard]] size_t getSize() const noexcept;

            /**
            * @brief Get the MemoryPoolBase Element Size
            *
            * This operation retrieves the size of the Elements specified at time of
            * construction. It delegates to the hidden implementation for the information.
            *
            * @return Returns the MemoryPoolBase::Imple element size specified at the time of construction.
            */
            [[nodiscard]] size_t getElementSize() const noexcept;

            /**
            * @brief Get the MemoryPoolBase Element Size
            *
            * This operation retrieves the padded size of the Elements managed by MemoryPoolBase determined at time of
            * construction. The padding allows for each raw block obtained through `getRawBlock` to be architecturally
            * aligned. It delegates to the hidden implementation for the information.
            *
            * @return Returns the MemoryPoolBase::Imple padded element size determined at the time of construction.
            */
            [[nodiscard]] size_t getPaddedElementSize() const noexcept;

            /**
            * @brief Get the Running State Statistics
            *
            * This operation provides for performance monitoring of the MemoryPoolBase. The data returned
            * is an atomically captured snapshot of the RunningStateStats. The "low watermark",
            * compared to the size can provide an indication of the exhaustion level.
            *
            * @return Returns a snapshot of internal RunningStateStats.
            */
            [[nodiscard]] RunningStateStats getRunningStateStatistics() const noexcept;

        protected:
            /**
            * @brief A Memory Manager for Raw Memory.
            *
            * This class exists to ensure that no leakage of the raw memory occurs during "cooking" operations.
            * Whether "cooking" succeeds without throwing or not, raw memory is returned to the
            * raw queue within the implementation if not explicitly released.
            */
            struct RawMemoryManager
            {
                /**
                * @brief Constructor for RawMemoryManager.
                *
                * This constructor records both an instance of the MemoryPoolBase object that constructed us
                * and a pointer to raw memory.
                *
                * @param pThePool A pointer to the MemoryPoolBase object that instantiated us.
                * @param pTheRaw A pointer to the raw memory to return to the raw pool.
                */
                RawMemoryManager( MemoryPoolBase * pThePool, void * pTheRaw ) : pP{ pThePool }, pR{ pTheRaw } {}

                /**
                * @brief Destructor for RawMemoryManager.
                *
                * This destructor returns raw memory back to the raw pool unless it has been formally released
                * through the `release` operation.
                */
                ~RawMemoryManager() { if ( pP && pR ) pP->returnRawBlock( pR ); }

                /**
                * @brief Copy Constructor for RawMemoryManager Disallowed
                *
                * Copy construction is disallowed. Hence, this operation is deleted.
                *
                * @param another A reference to another RawMemoryManager instance.
                */
                RawMemoryManager( RawMemoryManager & another ) = delete;

                /**
                * @brief Copy Assignment Operator for RawMemoryManager Disallowed
                *
                * Copy assignment is disallowed. Hence, this operation is deleted.
                *
                * @param another A reference to another RawMemoryManager instance.
                */
                RawMemoryManager & operator=( RawMemoryManager & another ) = delete;

                /**
                * @brief The Release Operation
                *
                * This operation releases RawMemoryManager from responsibility of needing to return
                * raw memory back to the raw pool.
                */
                void release() { pR = nullptr; }

            private:
                /**
                * @brief A Reference to the MessagePoolBase that constructed us.
                *
                * This attribute maintains a reference to the MessagePoolBase object that constructed us.
                */
                MemoryPoolBase * pP{ nullptr };

                /**
                * @brief A Reference to the raw memory that we are managing.
                *
                * This attribute maintains a reference to the raw memory that we are managing.
                */
                void * pR{ nullptr };
            };

        private:
            /**
            * @brief The Hidden Implementation Instance
            *
            * This attribute stores an instance of our hidden implementation.
            */
            Imple * pImple;
        };

    }
}

#endif /* REISERRT_CORE_MEMORYPOOLBASE_HPP */
