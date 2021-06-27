# ReiserRT_Core
Frank Reiser's C++11 core components for multi-threaded, realtime 
embedded systems.

---
Contents:\
   [1. Library Functionality](#library-functionality)\
   [1a. MessageQueue](#messagequeue)\
   [1b. ObjectPool](#objectpool)\
   [1c. RingBufferGuarded](#ringbufferguarded)\
   [1d. Semaphore](#semaphore)\
   [1e. Mutex](#mutex)\
   [1f. RingBufferSimple](#ringbuffersimple)\
   [2. Supported Platforms](#supported-platforms)\
   [3. Example Usage](#example-usage)\
   [4. Building and Installation](#building-and-installation)
---

## Library Functionality
The library provides the following components (from top to bottom):  
MessageQueue, ObjectPool, RingBufferGuarded, Semaphore, Mutex and
RingBufferSimple. In order to experience the best results, your
processes should enable a realtime scheduler. Under Linux,
my "go to" scheduler is "SCHED_FIFO". Additionally, your process
threads should run at a priority levels appropriate for your
application. Regardless of scheduler used, the functionality of
the components is basically the same. The impact the scheduler has is
on the level of determinism that can be expected. Do not expect
realtime performance without a realtime scheduler and for harder
realtime requirements, a realtime operating system. 

### MessageQueue
MessageQueue provides for object-oriented
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
be thread safe against both, asynchronous and synchronous
processing, operating from different threads. 

In order to take advantage of dispatch locking, you must construct
your MessageQueue instance with the enableDispatchLocking argument
set to true. Otherwise, dispatch locking is not in effect.
Enabling dispatch locking imposes a small performance
penalty on your dispatch loop. You should only enable
this feature if you need to coordinate synchronous and 
asynchronous behavior for the particular MessageQueue use case.

With dispatch locking enabled, a dispatch lock can be taken as follows:
  ```
   void someSynchronousAPI_Function(...)
   {
      // Lock out asynchronous dispatch.
      auto dispatchLock = messageQueueInstance.getAutoDispatchLock();
   
      // Do stuff then return
      return;
   }
   ```
When the instance of the MessageQueue::AutoDispatchLock is destroyed,
the dispatch lock releases.
Note: A std::runtime_error exception will be thrown if dispatch locking
is not in effect upon invoking getAutoDispatchLock.

Note: It is not necessary to obtain a dispatch lock if all you are
doing is enqueueing a message. The enqueueing of a message is in itself,
thread safe and does not require a dispatch lock.

Note: MessageQueue usage should be a hidden detail of any
particular component. It is easily hidden behind operations 
provided by a class API, and serviced by an implementation thread.
Not hiding MessageQueue usage exposes your queue and messages and 
how they are used. Hiding usage with an
implementation and only allowing access through your API affords
total control.

### ObjectPool
Calling new and delete in a realtime application can
significantly impact determinism. ObjectPool provides
pre-allocated memory, for a fixed number of objects of some
base type. The quantity of which, is specified during
construction. Derived objects may also be created from
ObjectPool as long as you respect appropriate maximum object size.
Maximum object size, is also specified during construction.

The `ObjectPool<BaseType>::createObj<DerivedType>(constructArgs...)`
operation returns a specialized unique_ptr type with a custom
deleter associated. The DerivedType may be the same as BaseType,
unless of course, the BaseType is abstract.
When the unique_ptr returned by createdObj is destroyed,
the object memory gets returned to the ObjectPool without 
specific intervention. These operations are thread safe 
(create and destroy).

NOTE: ObjectPool should be instantiated early and owned
at a level, high enough within an application's architecture, so that
it will not go out of scope until all objects created are returned.
Failing to honor this requirement will result in undefined behavior.

The unique_ptr type returned by ObjectPool<BaseType> may be "forward
declared" as follows:
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
out of the gate, and the objects may require further muting.
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

### RingBufferGuarded
RingBufferGuarded provides a thread safe buffering mechanism
typically seen in producer/consumer patterns.
However, items put into or taken from RingBufferGuarded may only
be scalar values. To deal with objects larger than a scalar,
we use pointers to an object type since pointers are scalars.

When a RingBufferGuarded instance becomes full, invokers of
the `put` API will block, waiting on a non-full condition. 
Similarly, when a RingBufferGuarded instance becomes empty, invokers
of the `get` API will block, waiting on a non-empty condition.

Please see the implementation details of MessageQueueBase for
a use case. MessageQueueBase utilizes RingBufferGuarded to accomplish
its goals.

### Semaphore
Semaphore provides for a thread safe, counted resource management 
tool. This particular implementation supports both an 
unbounded maximum availability and a bounded maximum availability
also known as bipolar mode. 
The choice of which is made at time of construction.

The two primary operations of Semaphore are `give` and `take`.
The `take` operation will block when Semaphore instance's availability
count has reached zero. The `give` operation will block only 
in bipolar mode if the availability count has reached a maximum
availability. In unbounded mode, `give` will never block.
However, it will throw an exception if the available count reaches an
absolute numeric limit of 2 to the power of 32, less 1 
(roughly 4 billion).

Please see the implementation details of RingBufferGuarded for
a use case. RingBufferGuarded utilizes Semaphore in bipolar mode
to accomplish its goals.

### Mutex
The Mutex class exists to overcome a limitation with C++11 
`std::mutex`. Specifically, on POSIX system, `std::mutex` does not
initialize with a priority inherit protocol and there is no way
to overcome this limitation using the `native_handle` operation.
POSIX PTHREADS mutexes must be explicitly initialized with a mutex
attribute enabling priority inheritance. On POSIX conformant systems
with PTHREADS available, Mutex will initialize a native mutex using 
the priority inherit protocol. On non-POSIX conformant systems,
`std::mutex` will be used directly. In order to achieve an adequate
level of determinism. ReiserRT_Core should be utilized on a POSIX
conformant system.

Mutex implements the "duck type" operations required in order to
use it directly with `std::lock_guard` and `std::unique_guard`.
It also provides the `native_handle` operation. 
In order to use Mutex with condition variables,
`std::condition_variable_any` must be used. POSIX systems
with PTHREADS available can avoid this overhead by using
PTHREADS condition variables directly.

Please see the implementation details of Semaphore for a use case.
Semaphore utilizes Mutex with condition variables. The PTHREADS
code uses PTHREADS condition variables. The non-PTHREADS code
uses `std::condition_variable_any`.

### RingBufferSimple
The RingBufferSimple class is a minimal implementation of ring
buffer logic. It does not provide any form of thread safety nor
guards against under and overflow. It primarily exists for the
purposes of RingBufferGuarded.

## Supported Platforms
This is a CMake project and at present, GNU Linux is
the only supported platform. Code from this project exist
under Windows but, the CMake work is not quite in place yet.
Note: This project requires CMake v3.17 or higher.

## Example Usage
Right now there exists one example in the 'examples' folder. It is
titled "JobDispatcher". 
"JobDispatcher" starts a number of "JobTask" (threads), quantity
one less than the number of CPUs available. These "JobTasks" are
available for excepting job assignments from the "JobDispatcher".
When a "JobTask" completes a job, it communicates back to the
"JobDispatcher" that a job has completed. 
If the "JobDispatcher" has more jobs, it will
dispatch another to this now idle "JobTask". "JobDispatcher"
utilizes one "ObjectPool" instance, and a number of "MessageQueue"
instances in somewhat of an architecture. It runs for a
few minutes with 8 CPUs available. In the future, we will provide
an "install" option for "JobDispatcher" in order to provide the
CMake details of how to link up from an external application to
ReiserRT_Core.

Example usage can also be found in the various
tests that exist in the "tests" folder although the tests are
not good examples of putting together an architecture.

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
5) Install the library as follows (You'll most likely 
   need root permissions to do this):
   ```
   sudo cmake --install .
   ```
   