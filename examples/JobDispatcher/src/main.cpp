#include <iostream>

// This is the only mandatory thing to get the ObjectPoolPtrType template using clause declared.
// This allows ObjectPoolPtrType to used as a reference without the type (T) pointed to being
// fully declared.
#include <memory>

// Let us try a forward declaration of a Deleter and see if that will compile.
// Okay, this compiles.
namespace ReiserRT
{
	namespace Core
	{
		template< typename T >
		class ObjectPoolDeleter;

		template < typename T >
		using ObjectPoolPtrType = std::unique_ptr< T, ObjectPoolDeleter< T > >;
	}
}

// Forward delcare a type and an alias to the ObjectPointerType<Blah>
// Note that we have not defined Blah, nor ObjectPoolPtrType. 
// Only forward declared them both. 
class Blah;
using BlahPtrType = ReiserRT::Core::ObjectPoolPtrType< Blah >;

// Let us try to declare a function that takes a reference to said pointer type
void blahFunc( BlahPtrType && pBlah );

// Now let us declare Blah to be something that has a size greater than zero.
class Blah
{
public:
	Blah()
	{
		std::cout << "Blah Constructed!" << std::endl;
	}
	~Blah()
	{
		std::cout << "Blah Destroyed!" << std::endl;
	}

private:
	size_t block[4];		
};


// Cannot forward declare the ObjectPool itself. It's template has default parameters.
// Argh, I wish I could.
#if 0
namespace ReiserRT
{
	namespace Core
	{
		template 
	}
}
#else
#include "ObjectPool.hpp"
#endif

// Declare a function that take utilizes an ObjectPool< Blah > internally.
void foo( )
{
	ReiserRT::Core::ObjectPool< Blah > pool{ 4 };

	BlahPtrType pBlah = pool.createObj< Blah >();
		

}

#include "MessageQueue.hpp"

void bar()
{
    class MyMessage : public ReiserRT::Core::MessageBase
    {
    public:
        using Base = ReiserRT::Core::MessageBase;
	using Base::Base;

    protected:
        virtual void dispatch() { std::cout << "Dispatched....WoOT!!!\n"; }
    };

    ReiserRT::Core::MessageQueue mq{ 3, sizeof( MyMessage ), true };

    mq.emplace< MyMessage >();
    mq.getAndDispatch();


    auto adp = mq.getAutoDispatchLock();

}

#include "JobDispatcher.hpp"

int main()
{
	///@todo Get rid of foo and bar tests and all their dependencies. I'm running JobDispatcher now.
//	foo();
//	bar();

    JobDispatcher jobDispatcher{};
    jobDispatcher.activate();
    jobDispatcher.runJobs();
    jobDispatcher.deactivate();

    return 0;
}

