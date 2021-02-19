//
// Created by frank on 2/19/21.
//

#ifndef REISERRT_STARTINGGUN_H
#define REISERRT_STARTINGGUN_H


class StartingGun {
public:
    StartingGun();
    ~StartingGun();

    void pullTrigger();
    void waitForStartingShot();
    void reload();
    void abort();
    bool isAborted();

private:
    class Imple;
    Imple * pImple{nullptr};
};


#endif //REISERRT_STARTINGGUN_H
