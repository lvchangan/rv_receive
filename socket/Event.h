//
// Created by hq on 2020/8/13.
//

#ifndef INC_2000_ANDROID_CAST_TV_EVENT_H
#define INC_2000_ANDROID_CAST_TV_EVENT_H

#include <cstring>
#include <cstdint>
#include <pthread.h>
#include <sys/time.h>


class Event {
public:
    Event() {
        pthread_mutex_init(&this->mutex, nullptr);
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
        pthread_cond_init(&this->cond, nullptr);
        pthread_condattr_destroy(&attr);
    };

    ~Event() {
        pthread_cond_destroy(&this->cond);
        pthread_mutex_destroy(&this->mutex);
    };

    int Wait() {
        int ret;
        pthread_mutex_lock(&this->mutex);
        ret = pthread_cond_wait(&this->cond, &this->mutex);
        pthread_mutex_unlock(&this->mutex);
        return ret;
    }

    int Wait(int ms) {
        int ret;
        pthread_mutex_lock(&this->mutex);
        struct timespec tv;
        struct timeval now;
        gettimeofday(&now, NULL);
        tv.tv_sec = now.tv_sec;
        tv.tv_nsec = now.tv_usec * 1000;
        if (ms >= 1000) {
            tv.tv_sec += ms / 1000;
            ms = ms % 1000;
        }
        tv.tv_nsec = ms * 1000 * 1000;
        ret = pthread_cond_timedwait(&this->cond, &this->mutex, &tv);
        pthread_mutex_unlock(&this->mutex);
        return ret;
    }

    void Signal() {
        pthread_mutex_lock(&this->mutex);
        pthread_cond_signal(&this->cond);
        pthread_mutex_unlock(&this->mutex);
    }

private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

#endif //INC_2000_ANDROID_CAST_TV_EVENT_H
