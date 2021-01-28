cmake_minimum_required(VERSION 3.17)
project(ReiserRT)

set(CMAKE_CXX_STANDARD 14)

include_directories(ReiserRT)

#add_executable(ReiserRT
#        ReiserRT/PlatformDetect.hpp
#        ReiserRT/PriorityInheritMutex.cpp
#        ReiserRT/PriorityInheritMutex.hpp
#        ReiserRT/Semaphore.cpp
#        ReiserRT/Semaphore.hpp)

add_library( ReiserRT STATIC
        ReiserRT/PlatformDetect.hpp
        ReiserRT/PriorityInheritMutex.cpp
        ReiserRT/PriorityInheritMutex.hpp
        ReiserRT/Semaphore.cpp
        ReiserRT/Semaphore.hpp
        ReiserRT/RingBufferSimple.hpp
        ReiserRT/RingBufferGuarded.hpp
        ReiserRT/ObjectPool.hpp
        ReiserRT/ObjectQueue.cpp
        ReiserRT/ObjectQueue.hpp
        ReiserRT/MessageQueue.hpp)
