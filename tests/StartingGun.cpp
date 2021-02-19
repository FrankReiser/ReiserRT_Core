//
// Created by frank on 2/19/21.
//

#include "StartingGun.h"

#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace std;

class StartingGun::Imple
{
    friend class StartingGun;

    Imple() = default;
    ~Imple() = default;

    void pullTrigger()
    {
        lock_guard<mutex> lock(goCondMutex);
        goShot = true;
        goCond.notify_all();
    }

    inline bool isGoing() { return goShot; }

    void waitForStartingShot()
    {
        unique_lock<mutex> lock(goCondMutex);
        goCond.wait(lock, [this](){ return isGoing(); } );
    }

    void reload()
    {
        goShot = false;
        abortFlagRB.store(false);
    }

    mutex goCondMutex;
    condition_variable goCond;
    atomic<bool> abortFlagRB{false};
    bool goShot{ false };
};

atomic<bool> abortFlagRB{ false };


StartingGun::StartingGun() : pImple{ new Imple }
{
}

StartingGun::~StartingGun()
{
    delete pImple;
}

void StartingGun::pullTrigger()
{
    pImple->pullTrigger();
}

void StartingGun::waitForStartingShot()
{
    pImple->waitForStartingShot();
}

void StartingGun::reload()
{
    pImple->reload();
}

void StartingGun::abort()
{
    abortFlagRB.store( true );
}

bool StartingGun::isAborted()
{
    return abortFlagRB.load();
}
