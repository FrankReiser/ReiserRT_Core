/**
* @file JobData.hpp
* @brief The Specification file for some abstract JobData along with a estimated time generator.
* @authors Frank Reiser
* @date Created on June 21, 2021
*/


#ifndef TESTREISERRT_CORE_JOBDATA_H
#define TESTREISERRT_CORE_JOBDATA_H

#include <cstdint>

class JobDataEstimatedTimeGenerator
{
private:
    class Details;

public:
    using EstimatedTimeDataType = long;  // Greater than 45bit required for use in millisecond duration per std::chrono.

    JobDataEstimatedTimeGenerator();
    ~JobDataEstimatedTimeGenerator();

    EstimatedTimeDataType getEstimatedTime();
private:
    Details * pDetails;
};

class JobData {
public:
    explicit JobData( JobDataEstimatedTimeGenerator * pTheEstTimeGen, unsigned theTaskId, unsigned theJobId );

private:
    JobDataEstimatedTimeGenerator * pEstTimeGenerator;

public:
    const JobDataEstimatedTimeGenerator::EstimatedTimeDataType estimatedEffortMSecs;
    const unsigned taskId;
    const unsigned jobId;
};

#endif //TESTREISERRT_CORE_JOBDATA_H
