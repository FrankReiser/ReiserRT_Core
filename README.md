# ReiserRT_Core
Frank Reiser's C++11 core components for multi-threaded, realtime 
embedded systems.

## Library Functionality
Currently, the library provides two primary components, 
ObjectPool and MessageQueue. These two components are built on top
of various subcomponents which are exported for potential reuse.
The subcomponents are Semaphore, RingBufferSimple, RingBufferGuarded
and ObjectQueue. In order to experience the best results, your
processes should enable a realtime scheduler. Under Linux,
my preferred scheduler is "SCHED_FIFO". Also, your
threads should run at a priority level appropriate for your
application. Regardless of scheduler used, the functionality of
the components are basically the same. The impact being
only on the level of determinism that can be achieved.

### ObjectPool
Calling new and delete in a realtime application can
significantly impact determinism. ObjectPool provides 
pre-allocated memory, for a fixed number of objects of some
base type. The quantity of which is specified during 
construction. Derived objects may also be created from
ObjectPool as long as the appropriate maximum object size
is not exceeded. This is also specified during construction.
The `ObjectPool<BaseType>::createObj<DerivedType>(constructArgs...)`
operation returns specialized unique_ptr types with a custom 
deleter associated. The DerivedType can be the same as BaseType,
unless of course, the BaseType is purely abstract.
When the unique_ptr returned by createdObj, is destroyed,
the object memory is automatically returned to the ObjectPool.
These operations are thread safe (create and destroy).

NOTE: ObjectPool is intended to be instantiated early and owned
at a level high enough within an application's architecture so that
it will not go out of scope until all objects created from it
have been returned. Failing to honor this will result
in undefined behavior.

The unique_ptr type returned by ObjectPool<BaseType> may be forward
declared as follows:
   ```
   #include <ObjectPoolFwd.hpp>
   
   // Forward, or fully declare your BaseType and ObjectPool's pointer type
   class MyBase;
   using MyBasePtrType = ReiserRT::Core::ObjectPoolPtrType<MyBase>;
   
   // Now you can use reference types of MyBasePtrType just like
   // any other forward declared type.
   ```
You can also specify a constant type as such `ObjectPoolPtrType<const T>`
if necessary although I seldom do so because, 
I often find I need to utilize use shared pointers.
The unique_ptr can be converted to a shared_ptr<const T> readily.
I find the `const` most useful in these shared_ptr cases because, unless `T` 
is thread-safe itself, shared data can cause much pain. Below is
an example of how to do this with the MyBasePtrType declared in the 
previous example:
   ```
   // Forward declare a vanilla shared pointer to constant MyBase.
   // This snippet could potentially be added to the previous example
   // and made into a separate include file.
   using MyBaseSharedPtrType = std::shared_ptr<const MyBase>;
   ```
   
   ```
   // To make an assignment, forward declaration alone will not cut
   // it. This would happen inside implementation code that is aware
   // of full details. Note: explicit move may or may not be required.
   // The value returned from ObjectPool<T>::createObj(...) can be
   // directly converted.
   MyBaseSharedPtrType sharePtr = std::move(myBaseObjectPtrTypeInstance);
   ```

### MessageQueue
MessageQueue provides for object oriented
inter-thread communications. Primarily, it provides a means
for which a high-priority task, handling a realtime interface,
can do necessary work and then hand off the rest to a lesser priority
task. In order to utilize MessageQueue, you need to derive
custom messages from MessageBase and override the dispatch
function. MessageQueue makes extensive use of ObjectPool
and ObjectQueue to accomplish its goals.

Messages are enqueued with either the
`MessageQueue::put< DerivedMsgType >( derivedType && )` or the
`MessageQueue::emplace< DerivedMsgType >( constructorArgs... )`.
With the put operation, you construct a message on the
stack and "move" that message into the queue. With
the emplace operation, the message is constructed right on a
pre-allocated block. Therefore, emplace is more direct and efficient.

MessageQueues are serviced with the
`MessageQueue::getAndDispatch(...)` operation. 
This operation should be called by an independent, implementation
thread as it will block when no messages are enqueued.
This provides for asynchronous message processing.
You are in total control as to what your various dispatch
overrides do.

MessageQueue usage should be a hidden detail of any
particular component. This is easily
hidden behind operations provided by
a class API and serviced by an implementation thread.
Not hiding MessageQueue usage will most likely, 
make public all your messages and having little control
over how they are used. Hiding usage with an implementation
and only allowing access through your API affords total control. 

If a software component, performing asynchronous processing with
MessageQueue, also needs to coordinate synchronous processing.
Asynchronous processing can be temporarily blocked. Taking
advantage of this feature allows reuse of the MessageQueue's
internal dispatch mutex. This allows all class attributes to
be thread safe with regards to asynchronous and synchronous
processing, operating from different threads. This can be accomplished
as follows:
  ```
   void someSynchronousAPI_Function(...)
   {
      // Lock out asynchronous dispatch.
      auto dispatchLock = messageQueueInstance.getAutoDispatchLock();
   
      // Do stuff then return
      return;
   }
   ```
When the instance of the MessageQueue::AutoDispatchLock goes out
of scope, the dispatch lock is released.

## Supported Platforms
This is a CMake project and at present, GNU Linux is
the only supported platform. Code from this project exist
under Windows but the CMake work is not quite in place yet.
Note: This project requires CMake v3.17 or higher.

## Example Usage
Example hookup and usage can be found in the various
tests that are present in the tests folder. Better examples
are planned. Stay tuned.

## Building and Installation
Roughly as follows:
1) Obtain a copy of the project
2) Create a build folder within the project root folder.
3) Switch directory to the build folder and run the following 
   to configure and build the project for you platform:
   ```
   cmake ..
   cmake --build .
   ```
4) Test the library
   ```
   ctest
   ```
5) Install the library as follows (You'll need root permissions)
   to do this:
   ```
   sudo cmake --install .
   ```
