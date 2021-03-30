/**
* @file ObjectPool.hpp
* @brief The Specification for a Specialized Object Pool Classes.
* @authors Frank Reiser
* @date Created on Mar 24, 2021
*
* @note Factored out from what is now ObjectPoolBase.hpp (Was ObjectPool.hpp). See history on that file for
* prior changes to template class ObjectPool now contained within.
*/

#include "ObjectPoolBase.hpp"
#include "ObjectPoolFwd.hpp"

#ifndef REISERRT_CORE_OBJECTPOOL_HPP
#define REISERRT_CORE_OBJECTPOOL_HPP

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief A Generic Object Pool Implementation with ObjectFactory Functionality
        *
        * This template class provides a compile-time, high performance, generic object factory from a preallocated
        * memory pool. The implementation relies heavily on C++11 meta-programming capabilities to accomplish its goal
        * of compile-time polymorphism. The RingBufferSimple class of the same namespace
        * is utilized extensively. Types created and delivered from this implementation are encapsulated
        * inside of a C++11 unique_ptr with a custom "Deleter" which automatically returns memory to this pool when the
        * unique_ptr is destroyed. Please @see ObjectPoolDeleter for details.
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
        template < typename T >
        class ObjectPool : public ObjectPoolBase
        {
        public:
            /**
            * @brief The Return Value Type
            *
            * This type definition describes the unique_ptr specialization that we return on a createObj invocation.
            * The details can be found within "ObjectPoolFwd.hpp".
            */
            using ObjectPtrType = ObjectPoolPtrType< T >;

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
            * @param minTypeAllocationSize. This minimum size for the elements managed by the ObjectPool.
            * This defaults to the size of type T but may be larger to accommodate the creation of derived types.
            * This value is clamped to be no less than the size of type T.
            */
            explicit ObjectPool( size_t requestedNumElements, size_t minTypeAllocSize = sizeof( T ) )
                : ObjectPoolBase{ requestedNumElements, std::max( minTypeAllocSize, sizeof( T ) ) }
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
            ~ObjectPool() = default;

            /**
            * @brief The createObj Variadic Template Operation
            *
            * This template operation provides for any derived type D of base type T to be generically constructed
            * using whatever parameters satisfy a particular constructor overload assuming it will fit into our
            * pre-sized arena blocks. This includes type T itself.
            *
            * The operation makes use of C++11 meta-programming capabilities and a variadic template specification
            * to inline the construction using "perfect forwarding" of the arguments. Objects created are constructed
            * using an "in-place" new operation, into memory blocks retrieved from the base class implementation.
            *
            * @tparam D An argument type derived from type T or type T itself.
            * It must be nothrow destructible and if type D is derived from type T,
            * then type T must specify a virtual destructor. Usage violations will be detected at compile time.
            * @tparam Args Zero or more arguments necessary to satisfy a particular type D constructor overload.
            *
            * @param args The actual arguments to be forwarded by the compiler to the deduced constructor for type D.
            *
            * @return This operation returns the new object constructed, wrapped within a std::unique_ptr,
            * associated with our custom Deleter. This unique_ptr is aliased as ObjectPtrType.
            *
            * @throw Throws std::underflow_error on memory pool exhaustion.
            * @throw Throws std::runtime_error if the size of type D exceeds the size of elements managed by the ObjectPool.
            * The element size is that requested during ObjectPoolConstruction with the minTypeSizeAlloc parameter
            * plus any alignment padding added by the implementation.
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

                // Type D must be nothrow_destructible.
                static_assert( std::is_nothrow_destructible< D >::value, "Type D must be nothrow destructible!!!" );

                // Get raw buffer from ring. This could throw underflow if pool is exhausted.
                void * pRaw;
///@todo I have to rethink this whole thing. Played around with throw_with_nested but that was for the Windows specific.
///I am not going to try and accomplish this right now. It is somewhat borked and may not last the test of time.
#if 0
                try
                {
                    pRaw =  getRawBlock();
                }
                catch ( const std::underflow_error & e )
                {
                    // Throw more informative underflow_error exception.
                    ///@todo This is Windows specific! I need what I did for GCC here and platform specific.
                    ///Also, figure out what is common between the two and maybe put this in a cpp file to invoke.
                    ///GNU C requires the inclusion of cxxabi.h for this to work. Can I make this platform specific
                    ///within an implementation file.
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
                    std::throw_with_nested( std::underflow_error( exceptionText ) );
                }
#else
                // Before we even bother getting a raw block of memory, we will validate that
                // the type being created will fit in the block.
                if ( getElementSize() < sizeof( D ) )
                    throw std::runtime_error( "ObjectPool::createObj: The size of type D exceeds maximum element size" );

                // Obtain a raw block of memory to cook.
                pRaw =  getRawBlock();
#endif

                // I could have used a unique_ptr to pull this trick off, but it isn't pretty. C-Tidy doesn't like
                // it and this is clean and lean. So, I just rolled my own again.
                struct RawMemoryManager {
                    RawMemoryManager( ObjectPool< T > * pThePool, void * pTheRaw ) : pP{ pThePool }, pR{ pTheRaw } {}
                    ~RawMemoryManager() { if ( pP && pR ) pP->returnRawBlock( pR ); }

                    void release() { pR = nullptr; }
                private:
                    ObjectPool< T > * pP{ nullptr };
                    void * pR{ nullptr };
                };

                // Wrap up the raw memory in our manager class so that it can be properly return to the pool
                // should something go wrong.
                RawMemoryManager rawMemoryManager{ this, pRaw };

                // Cook directly on raw and if construction doesn't throw, release managed pointer's ownership.
                // Ownership has been successfully transferred to pCooked.
                T * pCooked = new ( pRaw )D{ std::forward<Args>(args)... };
                rawMemoryManager.release(); // CLang C-Tidy warns we have not stored the value returned by release. Ignore.

                // Wrap for delivery.
                return ObjectPtrType{ pCooked, std::move(createDeleter<T>() ) };
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
        };
    }
}

#endif //REISERRT_CORE_OBJECTPOOL_HPP
