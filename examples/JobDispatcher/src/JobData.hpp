/**
* @file JobData.hpp
* @brief The Specification file for some abstract JobData along with a estimated time generator.
* @authors Frank Reiser
* @date Created on June 21, 2021
*/

#ifndef TESTREISERRT_CORE_JOBDATA_H
#define TESTREISERRT_CORE_JOBDATA_H

#include <cstdint>

/**
 * @brief The JobData Estimated Time Generator.
 *
 * This class simply generates a random number of milliseconds between 10000 and 20000 (10 to 20 seconds)
 * It exists so that a single instance of it may be created and reused to each JobData instance that requires
 * a random completion time during JobData construction.
 */
class JobDataEstimatedTimeGenerator
{
private:
    /**
     * @brief Forward Declaration of Hidden Details.
     *
     * Along the lines of the PIMPLE idiom (pointer to hidden implementation), we are going to hide our
     * our details. It is not really an implementation it is just some details which is why it is named "Details"
     * Rational is that we do not to create dependencies for client's use of our interface if we can avoid it.
     */
    class Details;

public:
    /**
     * @brief Our EstimatedTimeDataType.
     *
     * We alias our EstimatedTimeDataType so everyone is on the same page as to what it really is.
     * It needs to be greater than 45bit for use in millisecond duration per std::chrono.
     */
    using EstimatedTimeDataType = long;

    /**
     * @brief Constructor for our JobData Estimated Time Generator
     *
     * The constructor instantiates an instance of our hidden details from the standard heap and assign that
     * to our pDetails attribute.
     */
    JobDataEstimatedTimeGenerator();

    /**
     * @brief Destructor for our JobData Estimated Time Generator
     *
     * The destructor destroys our hidden details and returns memory to the standard heap.
     */
    ~JobDataEstimatedTimeGenerator();

    /**
     * @brief Get the Estimated Time
     *
     * This operation will deliver a random number of milliseconds between 10000 and 20000 (10 to 20 seconds).
     *
     * @return Returns a random number of milliseconds between 10000 and 20000 (10 to 20 seconds).
     */
    EstimatedTimeDataType getEstimatedTime();

private:
    /**
     * @brief Pointer to Hidden Details
     *
     * This attribute will point to our hidden details after construction.
     */
    Details * pDetails;
};

/**
 * @brief The Job Data.
 *
 * This fictitious Job Data is extremely contrived. It carries a task and a job identifier along with an estimated
 * time of completion in milliseconds. It could carry what ever is necessary for a particular use case. It will
 * be passed off to worker tasks from a job dispatcher. The worker tasks will pretend to actually do work on the job
 * by simply waiting for the estimated completion time to expire Then it will notify the dispatcher that the job
 * has been completed.
 */
class JobData {
public:

    /**
     * @brief Constructor for Job Data
     *
     * This constructor records a task and job identifier and uses the JobData estimated time
     * generator to initialize an estimated effort in milliseconds.
     *
     * @param pTheEstTimeGen The address of an instance of JobDataEstimatedTimeGenerator.
     * @param theTaskId The Task Id which will perform the job.
     * @param theJobId The Job Id of the job.
     */
    explicit JobData( JobDataEstimatedTimeGenerator * pTheEstTimeGen, unsigned theTaskId, unsigned theJobId );

public:
    /**
     * @brief Estimated Effort In Milliseconds
     *
     * This represents tan estimated time for completion for the job. It is assigned during construction.
     */
    const JobDataEstimatedTimeGenerator::EstimatedTimeDataType estimatedEffortMSecs;

    /**
     * @brief The Task Identifier
     *
     * This is our task identifier. It is assigned during construction.
     */
    const unsigned taskId;

    /**
     * @brief The Job Identifier
     *
     * This is our Job identifier. It is assigned during construction.
     */
    const unsigned jobId;
};

#endif //TESTREISERRT_CORE_JOBDATA_H
