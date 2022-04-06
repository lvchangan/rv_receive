/**
 * 环形缓存区域的实现
 *
 * 注意：保存是按照包来存入、读取的，
 * 每次保存前都会在前面加入 int32_t 来指明本次包的大小
 */
#include <assert.h>
#include "RingBuffer.h"
#include "log.h"
#include "Message.h"

struct Pack {
    uint32_t size;
    uint8_t data[0];
};
#define SIZE_RINGHEAD   sizeof(Pack)

RingBuffer::RingBuffer(int size) : CAPACITY(size) {
    BUFFER = new uint8_t[CAPACITY];
    END = BUFFER + CAPACITY;
    readPtr = writePtr = BUFFER;
    usedSize = 0;
    pthread_mutex_init(&lock, nullptr);
}

RingBuffer::~RingBuffer() {
    delete BUFFER;
    pthread_mutex_destroy(&lock);
}

/**
 * 获取当前剩余的大小
 * @return
 */
int32_t RingBuffer::GetFreeSize() {
    return CAPACITY - usedSize;
}

/**
 * 情况缓存，抛弃所有已保存的
 */
void RingBuffer::Free() {
    Locker locker(&lock);
    usedSize = 0;
    readPtr = writePtr = BUFFER;
}

/**
 * 写入到缓存区域
 * @param data 需要写入的数据
 * @param offset 偏移
 * @param len 数据长度
 * @return >=0 表示写入成功
 * <0表示写入失败，一般是空间不足，绝对值为需要的字节数
 */
int RingBuffer::Write(void *data, int len) {
    Locker locker(&lock);
    const int SRCSIZE = SIZE_RINGHEAD + len;
    const int leave = CAPACITY - usedSize - SRCSIZE;
    if (leave < 0) {
        return leave;
    }
    uint8_t *ptr = writePtr;
    Pack pack;
    pack.size = (uint32_t) len;
    //先写入头
    ptr = WriteInner(ptr, &pack, SIZE_RINGHEAD);
    ptr = WriteInner(ptr, data, len);
    writePtr = ptr;
    usedSize += (SIZE_RINGHEAD + len);
    return len;
}

/**
 * 读取
 * @param dest 将数据读取到这里
 * @param size dest 缓存的大小
 * @return
 * >0: 读取到的字节数。
 * 0: 无数据可读
 * <=0 失败表示需要的字节数
 */
int RingBuffer::Read(void *dest, int size) {
    Locker locker(&lock);
    if (usedSize == 0) {
        return 0;
    }

    uint8_t *ptr = readPtr;
    Pack pack;
    memset(&pack, 0x00, sizeof(pack));
    ptr = ReadInner(ptr, (uint8_t *) &pack, SIZE_RINGHEAD);
    if (size < (int)(pack.size)) {
        return 0 - pack.size;
    }
    ptr = ReadInner(ptr, (uint8_t *) dest, pack.size);
    readPtr = ptr;
    usedSize -= (pack.size + SIZE_RINGHEAD);
    return pack.size;
}

/**
 * 保留xx大小的空间
 * 如果当前剩余空间不足，则会从头开始抛弃包
 *
 * @param size 需要保留的空间大小
 */
void RingBuffer::Retain(int size) {
    if (size > CAPACITY) {
        return;
    }
    if (CAPACITY - usedSize >= size) {
        return;
    }
    Locker locker(&lock);
    while (CAPACITY - usedSize < size) {
        Pack *packageData = (Pack *) readPtr;
        const size_t PACKSIZE = packageData->size + SIZE_RINGHEAD;
        readPtr += PACKSIZE;
        if (readPtr >= END) {
            readPtr = BUFFER + (readPtr - END);
        }
        usedSize -= PACKSIZE;
    }
}

/**
 * 内部写入操作
 * 在调用前必须要执行：
 * 1.互斥锁
 * 2.判断剩余空间
 *
 * 注意：该函数仅仅将数据写入到环形缓存中，并不会更改 writePrt, usedSize 等任何变量
 *
 * @param ptr 往这个指针写入
 * @param data 写入的数据
 * @param len 写入的数据长度
 * @return 写入后的指针新的空闲位置的指针，可以使用这个指针继续写入数据
 */
uint8_t *RingBuffer::WriteInner(uint8_t *ptr, void *data, int len) {
    //判断是否到尾部了
    //写入到缓存
    if (END - ptr >= len) {
        //直接往尾部写入即可
        memcpy(ptr, data, len);
        return ptr + len;
    } else {
        uint8_t *src = (uint8_t *) data;
        //越过了尾部，需要写几节
        size_t first = END - ptr;
        memcpy(ptr, src, first);
        memcpy(BUFFER, src + first, len - first);
        return BUFFER + (len - first);
    }
}

/**
 * 内部读取操作
 * 在调用前必须要执行：
 * 1.互斥锁
 * 2.判断实际的已缓存的大小
 *
 * 注意：该函数仅仅是将缓存的读取到 dest 中，不会更改 readPtr, usedSize 等变量
 *
 * @param ptr 从这个指针开始读取
 * @param dest 读取到这里
 * @return
 */
uint8_t *RingBuffer::ReadInner(uint8_t *ptr, uint8_t *dest, int len) {
    if (END - ptr >= len) {
        memcpy(dest, ptr, len);
        return ptr + len;
    } else {
        //有回环了，需要复制两部分
        size_t first = END - ptr;
        memcpy(dest, ptr, first);
        memcpy(dest + first, BUFFER, len - first);
        return BUFFER + (len - first);
    }
}

/**
 * 执行测试用例
 * 主要是考虑超出边界长度以及分割的问题
 */
void RingBuffer::TestCase() {
    RingBuffer ringBuffer(1024);
    uint8_t data[80];
    uint8_t dest[80];
    SocketPackageData *packageData = (SocketPackageData *) data;
    SocketPackageData *packageDest = (SocketPackageData *) dest;
    packageData->magic = SOCKMAGIC_HEAD;
    packageData->dataSize = sizeof(data) - SOCKPACKAGEHEAD_SIZE;
#define ONEPACKAGE_SIZE     (sizeof(data)+SIZE_RINGHEAD)
    ringBuffer.Write(packageData, sizeof(data));
    assert (ringBuffer.usedSize == ONEPACKAGE_SIZE);

    ringBuffer.Write(packageData, sizeof(data));
    assert (ringBuffer.usedSize == ONEPACKAGE_SIZE * 2);
    ringBuffer.Free();
    assert(ringBuffer.usedSize == 0);

    ringBuffer.Write(packageData, sizeof(data));
    packageDest->magic = 0x00;
    ringBuffer.Read(dest, sizeof(dest));
    assert((packageDest->magic == SOCKMAGIC_HEAD));
    assert (ringBuffer.usedSize == 0);

    ringBuffer.Write(packageData, sizeof(data));
    assert (ringBuffer.usedSize == ONEPACKAGE_SIZE);
    ringBuffer.Write(packageData, sizeof(data));
    assert (ringBuffer.usedSize == ONEPACKAGE_SIZE * 2);
    ringBuffer.Write(packageData, sizeof(data));
    assert (ringBuffer.usedSize == ONEPACKAGE_SIZE * 3);
    ringBuffer.Read(dest, sizeof(dest));
    assert (ringBuffer.usedSize == ONEPACKAGE_SIZE * 2);
    ringBuffer.Read(dest, sizeof(dest));
    assert (ringBuffer.usedSize == ONEPACKAGE_SIZE);
    ringBuffer.Read(dest, sizeof(dest));
    assert (ringBuffer.usedSize == 0);

    for (int i = 0; i < 10; i++) {
        ringBuffer.Write(packageData, sizeof(data));
    }
    //写入的总数超出了缓存大小，循环缓存
    assert(ringBuffer.writePtr < ringBuffer.readPtr);
    assert(ringBuffer.usedSize == ONEPACKAGE_SIZE * 10);

    //逐一读取出来
    for (int i = 0; i < 10; i++) {
        packageDest->magic = 0x00;
        packageDest->dataSize = 0;
        int r = ringBuffer.Read(dest, sizeof(dest));
        assert(r == sizeof(dest));
        assert((packageDest->magic == SOCKMAGIC_HEAD));
        assert(packageDest->dataSize == sizeof(data) - SOCKPACKAGEHEAD_SIZE);
    }
#undef ONEPACKAGE_SIZE
}