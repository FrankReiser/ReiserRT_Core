//
// Created by frank on 2/19/21.
//

#include "StartingGun.h"

#include <mutex>
#include <condition_variable>

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
    }

    mutex goCondMutex;
    condition_variable goCond;
    bool goShot{ false };
};


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
