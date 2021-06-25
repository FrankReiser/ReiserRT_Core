//
// Created by frank on 6/21/21.
//

#ifndef TESTREISERRT_CORE_JOBTASK_HPP
#define TESTREISERRT_CORE_JOBTASK_HPP

#include "JobDataPtrTypeFwd.hpp"

// Forward Declare JobData and JobDataPtrType here thereby keeping weak relationships at the interface level.
class JobTask
{
private:
    class Imple;
public:
    using JobCompleteNotifier = std::function< void( JobDataPtrType && ) >;

    explicit JobTask( unsigned theTaskId );
    ~JobTask();

    template< typename Observer >
    void registerJobCompleteObserver( Observer && observer )
    {
        // Validate that the observer supports the required functor signature.
        static_assert( std::is_constructible< JobCompleteNotifier, Observer >::value,
                       "JobCompleteNotifier cannot be constructed from template argument 'Observer'!" );

        // Delegate to implementation.
        registerJobCompleteNotifier( JobCompleteNotifier{ std::forward< Observer >( observer ) } );
    }

    void activate();
    void deactivate();

    void doJob( JobDataPtrType && pJobData );

    unsigned getTaskId();


private:
    void registerJobCompleteNotifier( JobCompleteNotifier && notifier );

private:
    Imple * pImple;
};


#endif //TESTREISERRT_CORE_JOBTASK_HPP
