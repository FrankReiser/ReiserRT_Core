/**
* @file ObjectPool.hpp
* @brief The Specification for a Generic Object Pool.
* @authors Frank Reiser
* @date Created on Apr 9, 2015
*/

#ifndef OBJECTPOOL_H_
#define OBJECTPOOL_H_

#include "ReiserRT_Core/ReiserRT_CoreExport.h"

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
        * It provides a hidden implementation and the primary interface operations required for ObjectQueue.
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
            * @throw Throws std::underflow error if the memory pool has been exhausted.
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
            * @brief Get the ObjectPoolBase size
            *
            * This operation retrieves the fixed size of the ObjectPoolBase determined at the time of construction.
            * It delegates to the hidden implementation for the information.
            *
            * @return Returns the ObjectPoolBase::Imple fixed size determined at the time of construction.
            */
            size_t getSize() noexcept;

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

        /**
        * @brief A Generic Object Pool Implementation with ObjectFactory Functionality
        *
        * This template class provides a compile-time, high performance, generic object factory from a preallocated
        * memory pool. The implementation relies heavily on C++11 meta-programming capabilities to accomplish its goal
        * of compile-time polymorphism over dynamic dispatch. The RingBufferSimple class of the same namespace
        * is utilized extensively. Types created and delivered from this implementation are encapsulated
        * inside of a C++11 unique_ptr with a custom "Deleter" which automatically returns memory to this pool when the
        * unique_ptr is destroyed.
        *
        * @note Blocks of contiguous objects are not supported, only a single object per create request.
        *
        * @tparam T The type of object (base object implied) that will be created from the pool.
        * A whole hierarchy of objects types can be supported.
        *
        * @tparam minTypeAllocSize The minimum allocation block size. This defaults to the size of template parameter T.
        * In order to support the creation of derived types. The value should be the size of the largest derived type
        * that will be instantiated.
        *
        * @note This parameter will be rounded up to the next multiple of the architecture size so that all objects
        * created are architecture aligned.
        *
        * @warning The value must not be less than the size of template parameter T. This will be detected and reported
        * at compile-time as an error.
        */
        template < typename T, size_t minTypeAllocSize = sizeof( T ) >
        class ReiserRT_Core_EXPORT ObjectPool : public ObjectPoolBase
        {
            // You cannot specify a minTypeAllocSize less than the size of type T.
            static_assert( minTypeAllocSize >= sizeof( T ),
                    "Template parameter minTypeAllocSize must be >= sizeof( T )!!!" );

        public:
            /**
            * @brief Allocation Size Alignment Overspill
            *
            * This compile-time constant is essentially the remainder of the template parameter value minTypeAllocSize
            * divided by the size of the architecture. It is use to determine a padding to bring this size to the next
            * multiple of architecture size.
            */
            constexpr static size_t alignmentOverspill = minTypeAllocSize % sizeof( void * );

            /**
            * @brief The Padded Allocation Size
            *
            * This compile-time constant value is what we will allocate to each object created from the pool.
            * This ensures that all objects created, ultimately from the arena, are architecture aligned.
            */
            constexpr static size_t paddedTypeAllocSize = ( alignmentOverspill != 0 ) ?
                    minTypeAllocSize + sizeof( void * ) - alignmentOverspill : minTypeAllocSize;

        private:

            /**
            * @brief Deleter Functor
            *
            * This class provides a functor interface to be invoked when an object created by ObjectPool encapsulated
            * within a unique_ptr, is to be destroyed and subsequently returned to the memory pool.
            */
            class Deleter
            {
            private:
                /**
                * @brief Friend Declaration
                *
                * Our specialized parent ObjectPool is a friend and only it can invoke our "Qualified Constructor"
                * which specifies an instance of the specialization.
                */
                friend class ObjectPool;

                /**
                * @brief Qualified Constructor for Deleter
                *
                * Constructs a Deleter object with a pointer reference to the pool instance which created it.
                *
                * @param thePool A pointer to the object pool instance which invoke the operation.
                */
                explicit Deleter( ObjectPool * thePool ) noexcept : pool{ thePool } {}

            public:
                /**
                * @brief Default Constructor for Deleter
                *
                * Default construction required for an uninitialized instance of ObjectPtrType.
                */
                Deleter() noexcept = default;

                /**
                * @brief Default Destructor for Deleter
                *
                * We have no special needs. Therefore we request the default operation which does nothing.
                */
//                ~Deleter() noexcept = default;
                ~Deleter() = default;

                /**
                * @brief Copy Constructor for Deleter
                *
                * This is our copy constructor operation. An implementation must exist to copy Deleter objects
                * as this is what is a minimum requirement for unique_ptr ownership transfer and shared_ptr copying.
                *
                * @param another A reference to an object instance being copied.
                */
                Deleter( const Deleter & another ) noexcept : pool{ another.pool } {}

                /**
                * @brief Assignment Operator for Deleter
                *
                * This operation compliments our copy construtor. One shouldn't exist without the other.
                *
                * @param another A reference to an object instance being assigned from.
                */
                Deleter & operator=( const Deleter & another ) noexcept { pool = another.pool; return *this; }

                /**
                * @brief Functor Interface for Deleter
                *
                * This operation provides the interface necessary for instances of unique_ptr or shared_ptr to
                * destroy the object and return the memory to the pool.
                *
                * @param pT A pointer to the object to be destroyed and returned to the pool.
                * @warning Do not call with a nullptr. Type unique_ptr does not do so when it is destroyed.
                */
//                void operator()( T * pT ) noexcept { pT->~T(); if ( pool ) pool->returnBlock( pT ); }
                void operator()( T * pT ) { pT->~T(); if ( pool ) pool->returnBlock( pT ); }

            private:
                /**
                * @brief A Reference to Our Object Memory Pool
                *
                * This attribute records the ObjectPool instance that instantiated a Deleter.
                */
                ObjectPool * pool{ nullptr };
            };

        public:
            /**
            * @brief The Return Value Type
            *
            * This type definition describes the unique_ptr specialization that we return on a createObj invocation.
            * Of significance is that we associate a custom Deleter with the typed unique_ptr that we return.
            */
            using ObjectPtrType = std::unique_ptr< T, Deleter >;

            /**
            * @brief Default Constructor for ObjectPool
            *
            * Default construction of ObjectPool is disallowed. Hence, this operation has been deleted.
            */
            ObjectPool() = delete;

            /**
            * @brief Qualified Constructor for ObjectPool
            *
            * This qualified constructor builds an ObjectPool using the requestedNumElements argument value.
            * It delegates to its base class to satisfy construction requirements.
            *
            * @param requestedNumElements The requested ObjectPool size. This will be rounded up to the next whole
            * power of two and clamped within RingBuffer design limits.
            */
            explicit ObjectPool( size_t requestedNumElements )
              : ObjectPoolBase( requestedNumElements, paddedTypeAllocSize )
            {
            }

            /**
            * @brief Copy Constructor for ObjectPool
            *
            * Copying ObjectPool is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a ObjectPool of the same templated type.
            */
            ObjectPool( const ObjectPool & another ) = delete;

            /**
            * @brief Copy Assignment Operation for ObjectPool
            *
            * Copying ObjectPool is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a ObjectPool of the same templated type.
            */
            ObjectPool & operator =( const ObjectPool & another ) = delete;

            /**
            * @brief Move Constructor for ObjectPool
            *
            * Moving ObjectPool is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a ObjectPool of the same templated type.
            */
            ObjectPool( ObjectPool && another ) = delete;

            /**
            * @brief Move Assignment Operation for ObjectPool
            *
            * Moving ObjectPool is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a ObjectPool of the same templated type.
            */
            ObjectPool & operator =( ObjectPool && another ) = delete;

            /**
            * @brief Destructor for ObjectPool
            *
            * This destructor does little but delegate to the base class for the required clean-up.
            *
            * @note Destroying an ObjectPool while pointers to Objects are allocated and essentially 
             "loaned", would be a terrible thing to do. It will almost certainly lead to an exception
              being thrown.
            */
            ~ObjectPool() {}

            /**
            * @brief The createObj Variadic Template Operation
            *
            * This template operation provides for any derived type D of base type T to be generically constructed
            * using whatever parameters satisfy a particular constructor overload assuming it will fit into our
            * pre-sized arena blocks. This includes type T itself.
            *
            * The operation makes use of C++11 meta-programming capabilities and a variadic template specification
            * to inline the construction using "perfect forwarding" of the arguments. Objects created are constructed
            * using an "in-place" new operation, into a memory block retrieved from the base class implementation.
            *
            * @tparam D An argument type derived from type T or type T itself, guaranteed to fit within a memory block
            * from the arena. It must also be nothrow destructible and if type D is derived from type T,
            * then type T must specify a virtual destructor. Usage violations will be detected at compile time.
            * @tparam Args Zero or more arguments necessary to satisfy a particular type D constructor overload.
            *
            * @param args The actual arguments to be forwarded by the compiler to the deduced constructor for type D.
            *
            * @return This operation returns the new object constructed, wrapped within a std::unique_ptr,
            * associated with our custom Deleter. This unique_ptr is aliased as ObjectPtrType.
            *
            * @throw Throws underflow_error on memory pool exhaustion.
            * @note May throw other exceptions if the constructor of type D throws an exception.
            */
            template< typename D, typename... Args >
            ObjectPtrType createObj( Args&&... args )
            {
                // Type D must be derived from type T or the same as type T.
                static_assert( std::is_base_of< T, D >::value,
                        "Type D must be same the same type as T or derived from type T!!!" );

                // Type D must be the same type as type T or type T must specify virtual destruction.
                static_assert( std::is_same< D, T >::value || std::has_virtual_destructor< T >::value,
                         "Type D must be the same type as type T or type T must have a virtual destructor!!!" );

                // Type D must be the same type as type T or have a size less than that of the paddedTypeAllocSize.
                static_assert( std::is_same< D, T >::value || sizeof( D ) <= paddedTypeAllocSize,
                         "Type D must be the same type as type T or sizeof(D) must be <= padded allocation size!!!" );

                // Type D must be nothrow_destructible.
                static_assert( std::is_nothrow_destructible< D >::value, "Type D must be nothrow destructible!!!" );

                // Get raw buffer from ring. This could throw underflow if pool is exhausted.
                void * pRaw;
                try
                {
                    pRaw =  getRawBlock();
                }
                catch ( const std::underflow_error & )
                {
                    // Throw more informative underflow_error exception.
                    ///@todo This is Windows specific! I need what I did for GCC here and platform specific.
                    ///Also, figure out what is common between the two and maybe put this in a cpp file to invoke.
#if 0
                    int status;
                    char * pTypeName = abi::__cxa_demangle( typeid( T ).name(), 0, 0, &status );
                    char * pDerivedName = abi::__cxa_demangle( typeid( D ).name(), 0, 0, &status );
                    std::string typeName{ pTypeName };
                    std::string derivedName{ pDerivedName };
                    free( pTypeName );
                    free( pDerivedName );
                    std::string exceptionText{ "ObjectPool< " };
                    exceptionText += typeName;
                    exceptionText += ", ";
                    exceptionText += std::to_string( minTypeAllocSize );
                    exceptionText += " >::createObj< ";
                    exceptionText += derivedName;
                    exceptionText += " >( Args&&... args ) - Pool Exhausted!";
#else
                    const char * pTypeName = typeid( T ).name();
                    const char * pDerivedName = typeid( D ).name();
                    std::string exceptionText{ "ObjectPool< " };
                    exceptionText += pTypeName;
                    exceptionText += ", ";
                    exceptionText += std::to_string( minTypeAllocSize );
                    exceptionText += " >::createObj< ";
                    exceptionText += pDerivedName;
                    exceptionText += " >( Args&&... args ) - Pool Exhausted!";
#endif

                    throw std::underflow_error( exceptionText );
                }

                // A deleter and managed cooked pointer type.
                using DeleterType = std::function< void( void * ) noexcept >;
                using ManagedRawPointerType = std::unique_ptr< void, DeleterType >;

                // Wrap in a managed pointer
                auto deleter = [ this ]( void * p ) noexcept { this->returnRawBlock( p ); };
                ManagedRawPointerType managedRawPtr{ pRaw, std::ref( deleter ) };

                // Cook directly on raw and if construction doesn't throw, release managed pointer's ownership.
                T * pCooked = new ( pRaw )D{ std::forward<Args>(args)... };
                managedRawPtr.release();

                // Wrap cooked and deliver.
                return ObjectPtrType{ pCooked, Deleter{ this } };
            }

            /**
            * @brief Get the ObjectPool size
            *
            * This declaration brings the base class functionality into the public scope.
            */
            using ObjectPoolBase::getSize;

            /**
            * @brief Get the Running State Statistics
            *
            * This declaration brings the base class functionality into the public scope.
            */
            using ObjectPoolBase::getRunningStateStatistics;

        protected:
            /**
            * @brief Return Memory to the Pool
            *
            * This protected operation is used solely by our Deleter object to return memory to the ObjectPool after
            * an object's destruction. The pointer to this memory is put back into our RingBuffer for subsequent reuse.
            *
            * @param p A pointer to raw memory, that originally came from the ObjectPool.
            */
            void returnBlock( void * p ) noexcept
            {
                returnRawBlock( p );
            }
        };
    }
}

#endif /* OBJECTPOOL_H_ */
