/**
* @file JobDataPtrTypeFwd.hpp
* @brief The Forward Declaration of the JobDataPtrType we will be using.
* @authors Frank Reiser
* @date Created on June 23, 2021
*/


#ifndef TESTREISERRT_CORE_JOBDATAPTRTYPEFWD_HPP
#define TESTREISERRT_CORE_JOBDATAPTRTYPEFWD_HPP

#include "ObjectPoolFwd.hpp"
class JobData;
using JobDataPtrType = ReiserRT::Core::ObjectPoolPtrType<JobData>;


#endif //TESTREISERRT_CORE_JOBDATAPTRTYPEFWD_HPP
