/**
 * 由于Video数据包比较大，在发送端发送时会执行拆包
 * 使用本类完成所有分包组合的操作
 */

#ifndef _COMBINEPACKAGE_H_
#define _COMBINEPACKAGE_H_

#include <netinet/in.h>
#include "Message.h"
#include <string.h>

class CombinePackage {
public:
    CombinePackage();

    ~CombinePackage();

    void AllocBuffer(int size);

    void InitPackage(SocketPackageData *packageData, int onePackSize);

    bool PutPackage(SocketPackageData *packageData);

    uint32_t GetSeq() { return mAllPackData->seq; }

    int GetReceivedPackages() { return mRecvPackages; }

    /**
     * 是否接收完成所有的分包
     * @return
     */
    bool IsDone() const { return mRecvPackages >= mAllPackData->slices; }

    SocketPackageData *GetPackageData() const { return mAllPackData; }

    void GetSliceMask(uint32_t *masks) const;

    void GetRequestSliceMask(uint32_t *masks) const;

    int32_t GetDataType() const { return mAllPackData->type; }

    void ResetData() {
        mRecvPackages = 0;
        memset(mSlicesMask, 0x00, sizeof(mSlicesMask));
    }

public:
    static void AddSliceMask(int slice, uint32_t *masks);

    static bool IsSliceMask(int slice, uint32_t *masks);

private:
    uint8_t *mAllBuffers = nullptr;
    int mAllBufSize = 0;
    //实际上是指向 mAllBuffers
    SocketPackageData *mAllPackData = nullptr;
    /**
     * 记录的每个分包的大小
     */
    int mOnePackSize = 0;
    /**
     * 已经接收到数据包的个数
     */
    int mRecvPackages = 0;
    /**
     * 已接收到slice分片的掩码
     * 由于最大有256个分片，因此需要8个int32
     */
    uint32_t mSlicesMask[8];
};


#endif //_COMBINEPACKAGE_H_
