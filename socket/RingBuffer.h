/**
 * 环形缓冲区
 */

#ifndef SOCKET_RINGBUFFER_H
#define SOCKET_RINGBUFFER_H

#include <cstring>
#include <cstdint>
#include <pthread.h>

class RingBuffer {
public:
    RingBuffer(int size);
    ~RingBuffer();

    int32_t GetFreeSize();

    void Free();

    int Write(void *data, int len);

    int Read(void *dest, int size);

    void Retain(int size);

    static void TestCase();
private:
    const int CAPACITY;
    /**
     * 整个缓存
     */
    uint8_t *BUFFER;
    uint8_t *END;
    /**
     * 实际使用的指向
     */
    uint8_t *readPtr, *writePtr;
    /**
     * 当前已经使用了
     */
    int usedSize;
    /**
     * 多线程时锁
     */
    pthread_mutex_t lock;

private:
    uint8_t *WriteInner(uint8_t *ptr, void *data, int len);
    uint8_t *ReadInner(uint8_t *ptr, uint8_t *dest, int len);
private:
    class Locker {
        pthread_mutex_t *pLock;
    public:
        Locker(pthread_mutex_t *p) {
            pLock = p;
            pthread_mutex_lock(p);
        }

        ~Locker() {
            pthread_mutex_unlock(pLock);
        }
    };
};


#endif //SOCKET_RINGBUFFER_H
