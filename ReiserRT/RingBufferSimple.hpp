/**
* @file RingBufferSimple.hpp
* @brief The Specification file for RingBufferSimple
* @authors Frank Reiser
* @date Created on Apr 9, 2017
*/

#ifndef RINGBUFFERSIMPLE_H_
#define RINGBUFFERSIMPLE_H_

#include <type_traits>
#include <cstdint>
#include <stdexcept>
#include <cstddef>

namespace ReiserRT
{
    namespace Core
    {
        /**
        * @brief Base implementation class for all RingBufferSimple specializations
        *
        * This template class provides a for a simplistic, circular buffer.
        * As is, it is not thread safe and must be guarded from concurrent access of both
        * get and put operations (@see RingBufferGuardedBase).
        *
        * @tparam T The ring buffer element type.
        * @note Must be a scalar type (e.g., char, int, float or void pointer or typed pointer).
        */
        template< typename ScalarType >
        class RingBufferSimpleImple
        {
        private:
            // ScalarType must be a valid scalar type for rapid load and store operations.
            static_assert( std::is_scalar< ScalarType >::value,
                    "RingBufferImple<ScalarType> must specify a scalar type (which includes pointer types)!" );

            // ScalarType must not be constant. This is primarily necessary for put, which requires a write.
            static_assert( !std::is_const< ScalarType >::value,
                    "RingBufferImple<ScalarType> must specify a non-const scalar type (which includes pointer types)!" );

            /**
            * @brief Ring Buffer 32bit Counter Type
            *
            * This counter type is used to track get and put indices for all RingBuffer implementations.
            */
            using CounterType = uint32_t;

            /**
            * @brief Maximum Number of Elements
            *
            * This constant provides the maximum number of elements that may be put into a RingBuffer.
            *
            * @note This allows 1M of elements.  There's no reason it couldn't go higher (2^31 maybe even 2^32)
            * It would need to be tested at 2^32 as that is the absolute limit of our CounterType.
            */
            static constexpr CounterType maxElements = ( 1 << 20 ); // A quantity of 1M elements.

            /**
            * @brief Helper Operation used by numBitsForNE operation
            *
            * This recursive operation performs the actual work for the numBitsForNE operation.
            *
            * @param n The adjusted number of elements from that passed to the numBitsForNE operation.
            * @return The number of Bits required to index over the original N elements passed to the numBitsForNE
            * operation.
            */
            static CounterType _numBitsForNE( CounterType n )
            {
                return ( (n > 0) ? ( _numBitsForNE( n >> 1) + 1 ) : 0 );
            }

            /**
            * @brief Calculate a mask required to filter a counter to an index of N bits
            *
            * This operation calculates a mask for filtering a counter to an index of N bits.
            *
            * @param n The number of bits used for a counter.
            *
            * @return Returns the mask which can be utilized to filter a counter to an index of N bits.
            */
            static CounterType maskForNB( CounterType n )
            {
                return ( (n > 0) ? ( 1 << n ) - 1 : 0 );
            }

            /**
            * @brief Calculate the number of bits required an index an adjusted N elements
            *
            * This operation calculates the number of bits required to index (starting with zero)
            * N elements (e.g., 4 elements requires 2 bits (0-3).
            *
            * @note The requested number of elements is clamped between 2 and maxElements inclusive.
            *
            * @param requestedNumElements The number of elements.
            *
            * @return Returns the number of bits required to index over N elements.
            */
            static CounterType numBitsForNE( size_t requestedNumElements )
            {
                // Clamp requested number of elements to a minimum of 2 and a maximum of maxElements.
                CounterType n = CounterType( requestedNumElements < 2 ? 2 : requestedNumElements > maxElements ? maxElements : requestedNumElements );

                // Initially subtract 1 from non zero values to correctly calculate the number of bits.
                return _numBitsForNE( --n >> 1 ) + 1;
            }

            /**
            * @brief Friend Declaration
            *
            * Template class RingBufferSimpleBase is a friend and only it can invoked our member operations.
            */
            template< typename _ScalarType > friend class RingBufferSimpleBase;

        protected:
            /**
            * @brief Qualified Constructor for RingBufferSimpleImple
            *
            * This qualified constructor instantiates a RingBufferImple by first scrutinizing the requestedNumElements
            * argument. If less than two elements are requested, two will be allocated. If greater than maxElements is
            * requested, maxElements will be allocated. Everything in between will be rounded up to the next power of two.
            * Once the actual number of elements to be allocated is determined, a buffer for elements is allocated.
            *
            * @param requestedNumElements The number of elements requested. The actual size will be the next power of two.
            * @note 2 is the minimum and maxElements is the maximum. The requestedNumElements will be clamped to
            * this range during construction.
            */
            explicit RingBufferSimpleImple( size_t requestedNumElements )
                : getCount{ CounterType( ~0 ) }
                , putCount{ CounterType( ~0 ) }
                , numBits{ numBitsForNE( requestedNumElements ) }
                , numElementsMask{ maskForNB( numBits ) }
                , numElements{ numElementsMask + 1 }
                , pElementBuf{ new ScalarType[ numElements ] }
            {
            }

            /**
            * @brief Destructor for RingBufferSimpeImple
            *
            * The destructor returns the ring buffer element block to the standard heap.
            */
            ~RingBufferSimpleImple()
            {
                delete[] pElementBuf;
            }

            /**
            * @brief Get an Element From The RingBufferSimpleImple
            *
            * This operation attempts to get an element from the RingBufferImple and advance the getState.
            * @throw Throws std::underflow exception if there is no element available to fulfill the request.
            *
            * @return Returns an element from the RingBufferSimpleImple.
            */
            ScalarType get()
            {
                // To get, we cannot be empty.  If the get count is equal to the put count,
                // we are empty and will throw underflow.
                if ( ( getCount - putCount + numElements ) > numElementsMask )
                {
                    throw std::underflow_error{ "RingBufferSimpleImple::get() would result in underflow!" };
                }

                // If here, we were not empty. Incrementing the getCount,
                // fetch and return the element.
                return pElementBuf[ ++getCount & numElementsMask ];
            }

            /**
            * @brief Put an Element Into The RingBufferSimpleImple
            *
            * This operation attempts to put an element into the RingBufferImple and advance the putState.
            * @throw Throws std::overflow exception if there is no room left to fulfill the request.
            */
            void put( ScalarType val )
            {
                // To put, we cannot be full.  If the put total (sum indication of all put counters),
                // less the current get completion count, is greater than the number of elements mask
                // (number of elements less 1), then we are full and may throw overflow.
                if ( ( putCount - getCount ) > numElementsMask )
                {
                    throw std::overflow_error{ "RingBufferSimpleImple::put() would result in overflow!" };
                }

                // If here, we were not full. Load the value we are putting while incrementing the putCount.
                pElementBuf[ ++putCount & numElementsMask ] = val;
            }

            /**
            * @brief Get the Number Of Bits Operation
            *
            * This operation is primarily useful in validation of the implementation. It returns the
            * number of bits required to hold an index into an N element buffer.
            *
            * @return The number of bits required for a mask to determine an index into an N element buffer.
            */
            inline size_t getNumBits() const { return numBits; }

            /**
            * @brief Get the Size Operation
            *
            * This operation returns the number of elements actually allocated during construction
            * based on the requested number of elements.
            *
            * @return The number of elements that are available for put into a RingBufferImple from an empty state,
            * or got from a RingBufferImple from a full state.
            */
            inline size_t getSize() const { return numElements; }

            /**
            * @brief Get the Mask Operation
            *
            * This operation is primarily useful in validation of the implementation. It returns the
            * mask that is used to internally to transparently manage roll-over of the RingBufferImple.
            *
            * @return The mask that is used internally to transparently manage roll-over of the RingBufferImple.
            */
            inline size_t getMask() const { return numElementsMask; }

        private:
            /**
            * @brief Our Get Count
            *
            * This attribute tracks the get counter
            */
            CounterType getCount;

            /**
            * @brief Our Put Count
            *
            * This attribute tracks the put counter
            */
            CounterType putCount;

            /**
            * @brief The Number of Bits
            *
            * The number of bits required for a mask to determine an index into an N element buffer.
            */
            const CounterType numBits;

            /**
            * @brief The Number of Elements Mask
            *
            * The mask to determine an index into an N element buffer.
            */
            const CounterType numElementsMask;

            /**
            * @brief The Number of Elements.
            *
            * The number of elements that are available for put into RingBufferImple from an empty state,
            * or got from a RingBufferImple from a full state.
            */
            const CounterType numElements;

            /**
            * @brief The Element Buffer.
            *
            * This is the buffer space in where elements put into RingBufferImple are stored until subsequently retrieved.
            */
            alignas( ScalarType * ) ScalarType * pElementBuf;
        };

        /**
        * @brief RingBufferSimpleBase Class
        *
        * This template class provides a base for more specialized types. It maintains the
        * RingBufferSimpleImple of the same template argument type and provides access points to the
        * RingBufferSimpleImple private operations as we are it's only friend.
        *
        * @note All constructors have to be public to be inherited as public with using declaration for derived classes.
        *
        * @tparam T The ring buffer element type.
        * @note Must be a scalar type (e.g., char, int, float or void pointer or typed pointer).
        */
        template< typename T >
        class RingBufferSimpleBase
        {
        private:
            // T must be a valid scalar type for rapid load and store operations. Non-scalar types are supportable
            // through a type pointer. See class ObjectPool within this namespace for such a use case.
            static_assert( std::is_scalar< T >::value,
                    "RingBufferSimpleBase< T > must specify a scalar type (which includes pointer types)!" );

            /**
            * @brief Alias Type to RingBufferSimpleImple< T >
            *
            * This type provides a little "syntactic sugar" for the class.
            */
            using Imple = RingBufferSimpleImple< T >;


        public:

            /**
            * @brief Qualified Constructor for RingBufferSimpleBase
            *
            * This is our qualified constructor for RingBufferSimpleBase. It instantiates the implementation,
            * passing it the requestedNumElements argument.
            *
            * @param requestedNumElements The requested number of elements for RingBufferSimpleBase.
            */
            explicit RingBufferSimpleBase( size_t requestedNumElements ) : imple{ requestedNumElements }
            {
            }

            /**
            * @brief Copy Constructor for RingBufferSimpleBase
            *
            * Copying RingBufferSimpleBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a RingBufferSimpleBase of the same templated type.
            */
            RingBufferSimpleBase( const RingBufferSimpleBase & another ) = delete;

            /**
            * @brief Copy Assignment Operation for RingBufferSimpleBase
            *
            * Copying RingBufferSimpleBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another Another instance of a RingBufferSimpleBase of the same templated type.
            */
            RingBufferSimpleBase & operator =( const RingBufferSimpleBase & another ) = delete;

            /**
            * @brief Move Constructor for RingBufferSimpleBase
            *
            * Moving RingBufferSimpleBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a RingBufferSimpleBase of the same templated type.
            */
            RingBufferSimpleBase( RingBufferSimpleBase && another ) = delete;

            /**
            * @brief Move Assignment Operation for RingBufferSimpleBase
            *
            * Moving RingBufferSimpleBase is disallowed. Hence, this operation has been deleted.
            *
            * @param another An rvalue reference to another instance of a RingBufferSimpleBase of the same templated type.
            */
            RingBufferSimpleBase & operator =( RingBufferSimpleBase && another ) = delete;

            /**
            * @brief Destructor for RingBufferSimpleBase
            *
            * This is the destructor for RingBufferSimpleBase, I destroys and deletes the implementation instance thereby
            * returning it to the standard heap.
            */
            ~RingBufferSimpleBase() {}

        protected:
            /**
            * @brief The Get Operation
            *
            * This operation gets an element of type T from the implementation.
            *
            * @return Returns type T, returned from the implementation.
            */
            inline T get() { return imple.get(); }

            /**
            * @brief The Put Operation
            *
            * This operation puts an element of type T into the implementation.
            */
            inline void put( T val ) { imple.put( val ); }

            /**
            * @brief The Get Number of Bits Operation
            *
            * This operation returns the number of bits required by implementation.
            * It is primarily a validation operation.
            *
            * @return Returns the number of bits required by the implementation's numElementsMask attribute.
            */
            inline size_t getNumBits() const { return imple.getNumBits(); }

            /**
            * @brief The Get Size Operation.
            *
            * This operation returns the number of elements actually allocated during construction
            * based on the requested number of elements.
            *
            * @return The number of elements that are available for put into RingBufferSimpleImple from an empty state,
            * or got from a RingBufferSimpleImple from a full state.
            */
            inline size_t getSize() const { return imple.getSize(); }

            /**
            * @brief The Get Number of Bits Operation
            *
            * This operation returns the mask used to calculate indices by the implementation.
            * It is primarily a validation operation.
            *
            * @return Returns the number of bits required by the implementation's numElementsMask attribute.
            */
            inline size_t getMask() const { return imple.getMask(); }

        private:
            /**
            * @brief The Implementation
            *
            * This is our implementation object. It is simply aggregated. There is no need for another level of
            * indirection. The implementation already has one level.
            */
            Imple imple;
        };

        /**
        * @brief RingBufferSimple Class
        *
        * This template class provides a template for simple scalar types (not pointer types). It is derived from
        * RingBufferSimpleBase of the same template argument type which own an implementation instance.
        *
        * @tparam T The ring buffer element type (not for pointer types).
        * @note Must be a scalar type (e.g., char, int, float).
        */
        template< typename T >
        class RingBufferSimple : public RingBufferSimpleBase< T >
        {
        private:
            /**
            * @brief Alias Type to RingBufferSimpleBase< T >
            *
            * This type provides a little "syntactic sugar" for the class.
            */
            using Base = RingBufferSimpleBase< T >;

        public:
            /**
            * @brief Inherit Constructors from Base Class
            *
            * We add no additional attributes. This declaration specifies that we
            * want the Base version of constructors exposed as our own.
            */
            using Base::RingBufferSimpleBase;

            /**
            * @brief Inherit the Get Operation
            *
            * This declaration brings the get operation from our Base class into the public scope.
            */
            using Base::get;

            /**
            * @brief Inherit the Put Operation
            *
            * This declaration brings the put operation from our Base class into the public scope.
            */
            using Base::put;

            /**
            * @brief Inherit the Get Number of Bits Operation
            *
            * This declaration brings the getNumBits operation from our Base class into the public scope.
            */
            using Base::getNumBits;

            /**
            * @brief Inherit the Get Size Operation
            *
            * This declaration brings the getSize operation from our Base class into the public scope.
            */
            using Base::getSize;

            /**
            * @brief Inherit the Get Mask Operation
            *
            * This declaration brings the getMask operation from our Base class into the public scope.
            */
            using Base::getMask;
        };

        /**
        * @brief Specialization of RingBufferSimple for void Pointer Type
        *
        * This specialization of the RingBufferSimpleBase is specifically designed to handle void pointer
        * type values. It was designed to handle a wide variety of concrete typed pointers
        * that can transparently be cast back and forth with void pointers, thereby simplifying
        * the type pointer template. It inherits directly from the RingBufferSimpleBase.
        */
        template<>
        class RingBufferSimple< void * > : public RingBufferSimpleBase< void * >
        {
        private:
            /**
            * @brief Alias Type to RingBufferSimpleBase< void * >
            *
            * This type provides a little "syntactic sugar" for the class.
            */
            using Base = RingBufferSimpleBase< void * >;

        public:
            /**
            * @brief Inherit Constructors from Base Class
            *
            * We add no additional attributes. This declaration specifies that we
            * want the Base version of constructors exposed as our own.
            */
            using Base::RingBufferSimpleBase;

            /**
            * @brief Inherit the Get Operation
            *
            * This declaration brings the get operation from our Base class into the public scope.
            */
            using Base::get;

            /**
            * @brief Inherit the Put Operation
            *
            * This declaration brings the put operation from our Base class into the public scope.
            */
            using Base::put;

            /**
            * @brief Inherit the Get Number of Bits Operation
            *
            * This declaration brings the getNumBits operation from our Base class into the public scope.
            */
            using Base::getNumBits;

            /**
            * @brief Inherit the Get Size Operation
            *
            * This declaration brings the getSize operation from our Base class into the public scope.
            */
            using Base::getSize;

            /**
            * @brief Inherit the Get Mask Operation
            *
            * This declaration brings the getMask operation from our Base class into the public scope.
            */
            using Base::getMask;
        };

        /**
        * @brief A Partial Specialization for Concrete Type Pointers
        *
        * This partial specialization is provided for pointers to any type to be put into or retrieved from
        * a RingBufferSimple. It relies on its base class, RingBufferSimple< void * >, for all but a cast to and fro.
        */
        template< typename T >
        class RingBufferSimple< T * > : public RingBufferSimple< void * >
        {
        private:
            /**
            * @brief Alias Type to RingBufferSimple< void * >
            *
            * This type provides a little "syntactic sugar" for the class.
            */
            using Base = RingBufferSimple< void * >;

            /**
            * @brief Required Put Type
            *
            * Since the RingBufferImple cannot "put" into a constant buffer location because its internal representation is
            * not constant. This causes problems when type T is constant. Any constant qualifier must be removed.
            * Rest assured, the implementation will not mute any such data. Doing so would make it pretty much
            * useless.
            */
            using PutType = typename std::remove_const<T>::type *;

        public:
            /**
            * @brief Inherit Constructors from Base Class
            *
            * We add no additional attributes. This declaration specifies that we
            * want the Base version of constructors exposed as our own.
            */
            using Base::RingBufferSimple;

            /**
            * @brief The Get Operation
            *
            * This operation invokes the base class to retrieve a void pointer to the specified type
            * from the base RingBufferSimple.
            * The value is compile time converted to a pointer of the specified template type. Thereby,
            * adding no run-time penalty.
            *
            * @throw Throws std::underflow exception if there is no element available to fulfill the request (empty).
            *
            * @return Returns a pointer to an object of type T retrieved from the implementation.
            */
            inline T * get() { return reinterpret_cast< T* >( Base::get() ); }

            /**
            * @brief The Put Operation
            *
            * This operation invokes the base class to put a typed pointer value into the base RingBufferSimple.
            * Any constant specification is removed and the typed pointer is implicitly converted to a
            * void pointer at compile time yielding no run-time penalty.
            *
            * @throw Throws std::overflow exception if there is no room left to fulfill the request (full).
            *
            * @param p A pointer to the object to be put into the ring buffer implementation.
            */
            inline void put( T * p ) { Base::put( const_cast< PutType >( p ) ); }
        };

    }
}



#endif /* RINGBUFFERSIMPLE_H_ */
