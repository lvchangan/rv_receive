//
// Created by hq on 2021/2/1.
//

#include "CombinePackage.h"
#include "native.h"

CombinePackage::CombinePackage() {

}

CombinePackage::~CombinePackage() {
    if (mAllBuffers) {
        delete[] mAllBuffers;
    }
}

/**
 * 由于slice分片最多有256片，因此需要4个int32来保存
 * 该函数设置一个分片掩码
 * @param slice 分片序号，[0,255]
 * @param masks 需要设置的掩码保存数组
 */
void CombinePackage::AddSliceMask(int slice, uint32_t *masks) {
    int idx = slice / 32;
    int bit = slice % 32;
    masks[idx] |= (1 << bit);
}

/**
 * 该函数判断是否有这个掩码
 * @param slice 分片序号，[0,255]
 * @param masks 需要判断的掩码保存数组
 */
bool CombinePackage::IsSliceMask(int slice, uint32_t *masks) {
    if (!masks) {
        return true;
    }
    int idx = slice / 32;
    int bit = slice % 32;
    if(idx >= 8) return false;
    return (masks[idx] & (1 << bit)) != 0;
}

/**
 * 如果需要预先分配一个缓冲大小，请使用该函数
 * @param size 需要预先分配缓冲的大小
 */
void CombinePackage::AllocBuffer(int size) {
    if (mAllBuffers) {
        delete[] mAllBuffers;
    }
    size += sizeof(SocketPackageData);
    //4K对齐
    size = (size + 4096 - 1) & ~(4096 - 1);
    mAllBufSize = size;
    mAllBuffers = new uint8_t[mAllBufSize];
    mAllPackData = (SocketPackageData *) mAllBuffers;
    mAllPackData->seq = static_cast<uint32_t>(-1);
}

/**
 * 重新开始组合一个包
 * @param packageData 接收到的数据
 * @param onePackSize 每包的大小。用于在 mAllBuffers 中做偏移保存
 */
void CombinePackage::InitPackage(SocketPackageData *packageData, int onePackSize) {
    const int SIZE = packageData->slices * onePackSize;
    if (mAllBufSize < SIZE) {
        AllocBuffer(SIZE);
    }
    this->mOnePackSize = onePackSize;
    mAllPackData->seq = packageData->seq;
    mAllPackData->type = packageData->type;
    mAllPackData->slices = packageData->slices;
    mAllPackData->time = packageData->time;
    this->mRecvPackages = 0;
    memset(this->mSlicesMask, 0x00, sizeof(this->mSlicesMask));
    mAllPackData->dataSize = 0;
    PutPackage(packageData);
}


/**
 * 接收到一个分包后，加入进来并组合
 *
 * @param packageData Tcp/Udp 接收到的数据
 * @return true：正确的加入
 * false: 加入失败，比如不是同一个包下的分包
 */
bool CombinePackage::PutPackage(SocketPackageData *packageData) {
    if ((mAllPackData->seq != packageData->seq) || (mAllPackData->slices != packageData->slices)) {
        return false;
    }
    int offset = packageData->curSlice * mOnePackSize;
    memcpy(mAllPackData->data + offset, packageData->data, packageData->dataSize);
    if (!IsSliceMask(packageData->curSlice, this->mSlicesMask)) {
        this->mRecvPackages++;
        AddSliceMask(packageData->curSlice, mSlicesMask);
        mAllPackData->dataSize += packageData->dataSize;
        if (IsDone()) {
            int totalSize = mAllPackData->dataSize;
            mAllPackData->data[totalSize] = 0x000;
            mAllPackData->data[totalSize + 1] = 0x000;
            mAllPackData->data[totalSize + 2] = 0x000;
            mAllPackData->data[totalSize + 3] = 0x000;
        }
    }
    //ALOGI("HQ PutPackage mRecvPackages=%d, slices=%d, isDonw%d", this->mRecvPackages,
    //      mAllPackData->slices, IsDone());
    return true;
}

/**
 * 获取当前已经接收到的分片掩码
 * @param masks
 */
void CombinePackage::GetSliceMask(uint32_t *masks) const {
    memcpy(masks, this->mSlicesMask, sizeof(this->mSlicesMask));
}

/**
 * 获取还缺失的分片掩码。
 * 需要发送到Client请求
 */
void CombinePackage::GetRequestSliceMask(uint32_t *masks) const {
    for (int i = 0; i < sizeof(this->mSlicesMask) / sizeof(this->mSlicesMask[0]); i++) {
        masks[i] = ~this->mSlicesMask[i];
    }
}