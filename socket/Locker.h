//
// Created by hq on 2020/8/13.
//

#ifndef INC_2000_ANDROID_CAST_TV_LOCKER_H
#define INC_2000_ANDROID_CAST_TV_LOCKER_H


#include <cstring>
#include <cstdint>
#include <pthread.h>

class Locker {
public:
    Locker(pthread_mutex_t *p) {
        pMutex = p;
        pthread_mutex_lock(pMutex);
    }

    ~Locker() {
        pthread_mutex_unlock(pMutex);
    }

private:
    pthread_mutex_t *pMutex;
};


#endif //INC_2000_ANDROID_CAST_TV_LOCKER_H
