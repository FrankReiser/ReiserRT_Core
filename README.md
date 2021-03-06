# ReiserRT_Core
Frank Reiser's C++11 components for multi-threaded, realtime 
embedded systems.

## Preface
The topic of realtime programming is a complicated. This 
README file cannot begin to cover that in any sort of detail.
I will just touch on a few specific items below:

1. Determinism: What timeliness guarantees are required of the 
   system? What are the costs of failing to meet a specific
   time deadlines (Life, Limb or of less substantial consequence)?
   This is inline with various degrees of performance as 
   "hard to soft" real-time.

2. Scheduling: What type of schedulers will be at your
   disposal? Do they support a "first in - first out" priority
   scheme? Do they support "Priority Inheritance" in some form
   as a means of avoiding "Priority Inversion"?
   
3. Preemption: Does the operating system offer a fully preemptive
   kernel? 
   
Depending on how these questions are answered will assist in
defining your system requirements. Custom hardware may need to be
interfaced for certain hard real-time needs. An operating system
that supports a fully preemptive kernel may be required. Certain
functionality may need to be constrained to kernel space.
Then, there is always the "all of the above" approach.

## Some notes on Operating Systems

### Windows
Windows offers realtime process priority classes. However,
I would not kid myself too much here. To the best of my knowledge,
Windows does not offer a fully preemptive kernel. I am also
not sure about "Priority Inheritance".

I have deployed forms of the code on Windows 10 with great 
success though. It should also be noted that at the time of
this writing, I have not completed the CMake work
to completely deploy this project on Windows 10.
That task is a "work in progress".

### Linux (non-fully preemptive kernel)
I have tested this project under CentOS Linux 7.x and have deployed
various forms of this work on CentOS and RedHat variants with
great success. These types of standard Linux distributions offer
up POSIX pthreads and other threading primitives. C++11 threads
are implemented on top of POSIX pthreads on these type of systems.
Schedulers for realtime are provided in the form of SCHED_FIFO
and SCHED_RR (round-robin). POSIX also support a mutex protocol
to utilize "Priority Inheritance" to prevent a low priority thread
from unduly blocking a high priority thread. However, they do not
typically "out of the box", offer a fully preemptive kernel.
It should be noted that using these scheduler types on a non RTOS
may be subject to bandwidth limitations imposed by the kernel.
When awakened by the scheduler, do you work quickly and
go back to sleep. Heavy duty processing may need to be delegated
to background tasks utilizing SCHED_OTHER.

### Linux (fully preemptive kernel - RTOS)
I believe there are kernel patches available to accomplish this.
There are providers of such realtime operating systems. These
are typically built on top of another distribution and offer
numerous support features such as deep kernel tracing 
should that become necessary. This code has been deployed in
various forms on such Linux Real Time Operating Systems (RTOS)

# The ReiserRT_Core Project from the Top
At the top of this project are ObjectPools and MessageQueues. They
are built on top of lower level components contained within, that
can stand on their own. These will be discussed further down.

## Object Pools
Most realtime programs should avoid usage of the standard heap
like the plague. The standard heap is a system wide resource
and is a contentious place where blocking is likely occur.
Blocking on a "new" or "delete" operations will negatively 
impact system determinism. Object pools only utilize the heap
during initialization and destruction.

The templated ObjectPool class provides a unique factory pattern.
It is like an "abstract factory" except it is "compile time"
abstract. It employs variadic templates to create objects of any
particular base type within the bounds of a maximum derived
type size. You define the number of Objects, the maximum size
of the Objects and a particular object base class. Polymorphism
is not a requirement nor an impediment of you class hierarchy.
In essence, there is no need for a "createObj" type abstract of
your base class. ObjectPool addresses that for you.
ObjectPool instances should not be allowed to go out of scope
with Objects outstanding. They should be constructed early
and destroyed late at a level high enough to ensure this does
not occur.

The ObjectPool class createObj function returns a 
C++11 std::unique_ptr with a custom deleter type. When the
unique_ptr is no longer used, the memory consumed by the object
will return to the ObjectPool from whence it came, automatically.
The unique_ptr returned by createObj can be readily converted to a 
shared_ptr and has been used that way in much code I have 
deployed.

## Message Queues
The templated MessageQueue class provides a mechanism to push
work accomplished by one task onto another task for additional
processing. This affords a high priority task to do what is
required of it and to delegate any further work to a 
task of lesser priority. Internally it utilizes an ObjectPool
and an ObjectQueue (not yet discussed).

The MessageQueue defines the MessageBase class with
an abstract "dispatch" operation. It is up to the implementor
of such a MessageBase derived class to define what this means.
I have typically implemented these derivations as a
form of "double dispatch". First the derived dispatch and then
onto some other implementation.

MessageQueues will block the enqueueing of messages at a 
certain point if they are not being dispatched regularly.
Obviously this is undesirable and should be avoided by ensuring
adequate priority of whatever is dequeue-ing and dispatching
messages. MessageQueues will also block on the dequeue-ing and
dispatching of messages should the queue become empty.
This is a normal occurrence and expected if all is running
smoothly.

# The ReiserRT_Core Project from the Bottom
At the bottom of this project are the types used by the
main classes in this library.

## Counted Semaphores
The Semaphore class is a thread-safe "resource management" tool.
It is constructed with an initial count indicating the 
"resources available" which can be zero.
It supports two primary operations. These are wait and notify.
The wait will block if the available count becomes zero. At that
point, only a notify operation invoked by another thread
will allow a blocked wait to wake up. If the available count
is not already zero, wait will "not wait" and decrement the
available count. Notify on the other hand will never block.
It increments the available count.

## Ring Buffers
The ReiserRT_Core library offers two RingBuffer classes.
RingBufferSimple is an efficient primitive ring buffer
implementation. However, it in and of itself is not thread-safe.
RingBufferGuarded is built on top of RingBufferSimple and employs
a counted Semaphore to wait on an empty ring buffer.
RingBufferGuarded will not wait on attempting to "put" onto
a full ring buffer. That must be managed externally.

## Object Queues
The ObjectQueue template class provides for the simple enqueueing
and de-queueing of objects. It will block on attempts to overfill
it with objects and on attempts to get objects from an empty queue.
It is currently only used withing the MessageQueue implementation.
