#include "JobDispatcher.hpp"

int main()
{
    JobDispatcher jobDispatcher{};
    jobDispatcher.activate();
    jobDispatcher.runJobs();
    jobDispatcher.deactivate();

    return 0;
}

