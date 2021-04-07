# ReiserRT_Core
Frank Reiser's C++11 core components for multi-threaded, realtime 
embedded systems.

---
Contents:\
   [1. Library Functionality](#library-functionality)\
   [1a.  ObjectPool](#objectpool)\
   [1b.  MessageQueue](#messagequeue)\
   [2. Supported Platforms](#supported-platforms)\
   [3. Example Usage](#example-usage)\
   [4. Building and Installation](#building-and-installation)
---

## Library Functionality
Currently, the library provides two primary components, 
ObjectPool and MessageQueue. These two components are built on top
of various subcomponents which are exported for potential reuse.
These subcomponents are Semaphore, RingBufferSimple and RingBufferGuarded.
In order to experience the best results, your
processes should enable a realtime scheduler. Under Linux,
my "go to" scheduler is "SCHED_FIFO". Additionally, your
threads should run at a priority level appropriate for your
application. Regardless of scheduler used, the functionality of
the components is unaffected. The only impact the scheduler has is, 
the level of determinism that can be achieved.

### ObjectPool
Calling new and delete in a realtime application can
significantly impact determinism. ObjectPool provides 
pre-allocated memory, for a fixed number of objects of some
base type. The quantity of which is specified during 
construction. Derived objects may also be created from
ObjectPool as long as the appropriate maximum object size
is not exceeded. This is also specified during construction.
The `ObjectPool<BaseType>::createObj<DerivedType>(constructArgs...)`
operation returns a specialized unique_ptr type with a custom 
deleter associated. The DerivedType may be the same as BaseType,
unless of course, the BaseType is abstract.
When the unique_ptr returned by createdObj is destroyed,
the object memory is automatically returned to the ObjectPool.
These operations are thread safe (create and destroy).

NOTE: ObjectPool is intended to be instantiated early and owned
at a level, high enough within an application's architecture, so that
it will not go out of scope until all objects created from it
have been returned. Failing to honor this requirement will result
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
You can also specify constant types as such `ObjectPoolPtrType<const T>`
if necessary although I seldom do so because, it is too restrictive right
out of the gate.
This unique_ptr can be converted to a shared_ptr<const T> if needed.
I find the `const` most useful in these shared_ptr cases because, unless `T` 
is thread-safe itself, shared data can cause much pain. Below is
an example of how to do this with the MyBasePtrType declared in the 
previous example:
   ```
   // Forward declare a shared pointer to constant MyBase.
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
inter-thread communication. Primarily, it provides a means
for which a high-priority task, handling a realtime interface,
can do necessary work and then hand off the additional work to a lesser priority
task. Internally, it uses pre-allocated memory for it's buffering purposes
in order to avoid system heap usage, post initialization, as does ObjectPool.

In order to utilize MessageQueue, you will need to derive your own
custom messages based off of MessageBase and override the dispatch
function.

Messages are enqueued with either the
`MessageQueue::put< DerivedMsgType >( derivedType && )` or the
`MessageQueue::emplace< DerivedMsgType >( constructorArgs... )`.
With the put operation, you construct a message on the
stack and "move" that message into the queue. With
the emplace operation, the message is constructed right on a
pre-allocated block within MessageQueue. Therefore, emplace is
more direct and efficient.

MessageQueues are serviced with the
`MessageQueue::getAndDispatch(...)` operation. 
This operation should be called by an independent, implementation
thread as it will block when no messages are enqueued.
This provides a means for asynchronous message processing.
You are in total control as to what your various dispatch
overrides do.

If a software component, performing asynchronous processing with
MessageQueue, also needs to coordinate synchronous processing.
Asynchronous processing can be temporarily blocked. Taking
advantage of this feature allows reuse of the MessageQueue's
internal dispatch mutex. This allows a client's attribute data to
be thread safe with regards to asynchronous and synchronous
processing, operating from different threads. 

In order to take advantage of dispatch locking, you must construct your
MessageQueue instance with the enableDispatchLocking argument set to true.
Dispatch locking is not enabled by default as there is a small performance
penalty imposed on your dispatch loop to utilize it. You should only enable
this feature if you need to coordinate synchronous and asynchronous behavior.

If dispatch locking is enabled, a dispatch lock can be taken as follows
(note a std::runtime_error will be thrown if dispatch locking is not enabled):
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

Note: It is not necessary to obtain a dispatch lock if all you are
doing is enqueueing a message. The enqueueing of a message is in itself,
thread safe and does not require a dispatch lock.

Note: MessageQueue usage should be a hidden detail of any
particular component. It is easily hidden behind operations provided by
a class API and serviced by an implementation thread.
Not hiding MessageQueue usage may expose your messages and 
limit the control over how messages are used. Hiding usage with an
implementation and only allowing access through your API affords total control.

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
