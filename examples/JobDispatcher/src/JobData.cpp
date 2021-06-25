/**
* @file JobData.hpp
* @brief The Implementation file for some abstract JobData along with a estimated time generator.
* @authors Frank Reiser
* @date Created on June 21, 2021
*/

#include "JobData.hpp"

#include <random>       // For Random Number Generators and Distributions.

class JobDataEstimatedTimeGenerator::Details
{
private:
    friend class JobDataEstimatedTimeGenerator;

    Details()
            : jobDataRndNumberGen(std::random_device{}())  // Construct Temporary std::random_device and invoke it!
            , jobDataRndNumberDist{10000, 20000}        // 10 to 20 seconds in milliseconds.
    {
    }

    JobDataEstimatedTimeGenerator::EstimatedTimeDataType getEstimatedTime()
    {
        return jobDataRndNumberDist( jobDataRndNumberGen);
    }

    std::default_random_engine jobDataRndNumberGen;
    std::uniform_int_distribution<JobDataEstimatedTimeGenerator::EstimatedTimeDataType> jobDataRndNumberDist;
};

JobDataEstimatedTimeGenerator::JobDataEstimatedTimeGenerator()
    : pDetails{ new Details }
{
}

JobDataEstimatedTimeGenerator::~JobDataEstimatedTimeGenerator()
{
    delete pDetails;
}

JobDataEstimatedTimeGenerator::EstimatedTimeDataType
JobDataEstimatedTimeGenerator::JobDataEstimatedTimeGenerator::getEstimatedTime()
{
    return pDetails->getEstimatedTime();
}


JobData::JobData( JobDataEstimatedTimeGenerator * pTheEstTimeGen, unsigned theTaskId, unsigned theJobId )
    : pEstTimeGenerator{pTheEstTimeGen}
    , estimatedEffortMSecs(pEstTimeGenerator->getEstimatedTime())
    , taskId(theTaskId)
    , jobId(theJobId)
{
}

