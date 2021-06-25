/**
* @file JobDispatcher.hpp
* @brief The Specification for some abstract JobDispatcher
* @authors Frank Reiser
* @date Created on June 21, 2021
*/

#ifndef TESTREISERRT_CORE_JOBDISPATCHER_HPP
#define TESTREISERRT_CORE_JOBDISPATCHER_HPP

class JobDispatcher
{
private:
    class Imple;
public:
    JobDispatcher();
    ~JobDispatcher();

    void activate();
    void deactivate();

    // A runJobs function. We will runJobs synchronous with whatever calls us until we are done.
    void runJobs();

private:
    Imple * pImple;
};


#endif //TESTREISERRT_CORE_JOBDISPATCHER_HPP
