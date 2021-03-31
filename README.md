# ReiserRT_Core
Frank Reiser's C++11 core components for multi-threaded, realtime 
embedded systems.

## Library Functionality
Currently, the library provides two primary components. 
These are ObjectPool and MessageQueue. These are built on top
of various subcomponents which are exported for potential reuse.
These are Semaphore, RingBufferSimple, RingBufferGuarded and
ObjectQueue. In order to experience the full benefit, your
processes should enable a realtime scheduler. Under Linux,
the preferred scheduler would be "SCHED_FIFO". Also, your
threads would run at a priority level appropriate for your
application. Regardless of scheduler, the functionality of
the components is the same.

### ObjectPool
Calling new and delete in a realtime application can
significantly impact determinism. ObjectPool provides 
pre-allocated memory for a fixed number of objects of some
base type. The quantity of which is specified during 
construction. Derived objects may also be created from
ObjectPool as long as the appropriate maximum object size
is not exceeded. This is also specified during construction.
The `ObjectPool<BaseType>::createObj<DerivedType>(constructArgs...)`
operation returns specialized unique_ptr types with a custom 
deleter associated. The DerivedType can be the same as BaseType.
When the unique_ptr is destroyed. The object memory is returned
to the ObjectPool automatically. This operation is thread safe.

ObjectPool is intended to be instantiated early and owned
at a high level within the applications architecture so that
it will not go out of scope until all objects created from it
are returned to the it. Failing to honor this will result
in undefined behavior.

### MessageQueue
MessageQueue provides for object oriented
intra-thread communications. It provides a means
for which a high-priority task, handling a realtime interface,
can do necessary work and then hand off the rest to a lesser priority
task. In order to utilize MessageQueue, you need to derive
custom messages from MessageBase and override the dispatch
function. MessageQueue makes extensive use of ObjectPool
and ObjectQueue to accomplish its goals.

Messages are enqueued with either the
`MessageQueue::put< DerivedMsgType >( derivedType && )` or the
`MessageQueue::emplace< DerivedMsgType >( constructorArgs... )`.
With the put operation, you construct the message first on
your stack and it gets moved into the queue. With
the emplace operation, it gets constructed right on a
pre-allocated block. Therefore, emplace is more efficient.

Messages are dequeue with the
`MessageQueue::getAndDispatch(...)` operation. You have total 
control as to what your various dispatch overrides do.

MessageQueue usage should be a hidden detail of any
particular component that utilizes it. It is easily
hidden behind asynchronous interface operations provided by
a class API. Not hiding MessageQueue usage will most likely 
make public all your messages and having little control
over how they are used. Hiding it with an implementation
and only allowing access through your API affords total control. 

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
