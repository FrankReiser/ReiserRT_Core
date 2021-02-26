//
// Created by frank on 2/18/21.
//

#include "RingBufferSimple.hpp"

#include <iostream>

using namespace std;
using namespace ReiserRT::Core;

int main()
{
    int retVal = 0;
    do {
        // Create a ring buffer and verify that it has the correct size.  Ask for 3, should get 4
        RingBufferSimple<int> ringBuffer{3};
        if ( ringBuffer.getSize() != 4 )
        {
            cout << "RingBuffer should have a reported size of 4 and has a size of " << ringBuffer.getSize() << endl;
            retVal = 1;
            break;
        }

        // Attempt to get on empty ring buffer.  It should throw.
        try
        {
            ringBuffer.get();

            // If we make it here, it failed.
            cout << "RingBuffer should have thrown an exception on get attempt with empty ring buffer" << endl;
            retVal = 2;
            break;
        }
        catch (underflow_error&)
        {
            // If we make it here, it passed.
        }

        // Attempt to overrun ring buffer.  It should throw on the 5th "put" attempt.
        int i;
        for (i = 0; i != 10; ++i)
        {
            try
            {
                ringBuffer.put(i);
            }
            catch (overflow_error&)
            {
                break;
            }
        }
        if (i != 4)
        {
            cout << "RingBuffer should have thrown an exception on the 5th \"put\" attempt.  Iterator got to a value of " << i << endl;
            retVal = 3;
            break;
        }

        // Test that the contents of the ring buffer are correct and that it throws on the 5th "get" attempt.
        for (i = 0; i != 10; ++i)
        {
            try
            {
                int v = ringBuffer.get();
                if (size_t(v) != i)
                {
                    cout << "RingBuffer \"get\" should have returned " << i << " and returned " << v << endl;
                    retVal = 4;
                    break;
                }
            }
            catch (underflow_error&)
            {
                break;
            }
        }
        if (0 != retVal && i != 4)
        {
            cout << "RingBuffer should have thrown an exception on the 5th \"get\" attempt.  Iterator got to a value of " << i << endl;
            retVal = 5;
            break;
        }
        else if ( 0 != retVal)
            break;


    } while ( false );

    return retVal;
}

