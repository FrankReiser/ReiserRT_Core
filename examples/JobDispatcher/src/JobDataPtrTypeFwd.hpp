/**
* @file JobDataPtrTypeFwd.hpp
* @brief The Forward Declaration of the JobDataPtrType we will be using in this demo.
* @authors Frank Reiser
* @date Created on June 23, 2021
*/


#ifndef TESTREISERRT_CORE_JOBDATAPTRTYPEFWD_HPP
#define TESTREISERRT_CORE_JOBDATAPTRTYPEFWD_HPP

#include "ObjectPoolFwd.hpp"

/**
 * @brief Forward Declaration of JobData
 *
 * We forward declare JobData here for interfaces that only require a pointer or reference to JobData.
 */
class JobData;

/**
 * @brief Object Pool Pointer Type for JobData
 *
 * This is simply an alias to the data type (unique ptr with custom deleter) for JobData.
 */
using JobDataPtrType = ReiserRT::Core::ObjectPoolPtrType<JobData>;


#endif //TESTREISERRT_CORE_JOBDATAPTRTYPEFWD_HPP
