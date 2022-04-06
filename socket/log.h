//
// Created by hq on 2019/8/12.
//

#ifndef UDPDEMO_LOG_H
#define UDPDEMO_LOG_H

#ifndef LOGD
#define   LOGD(...)  printf(__VA_ARGS__)
#define   LOGI(...)  printf(__VA_ARGS__)
#define   LOGW(...)  printf(__VA_ARGS__)
#define   LOGE(...)  printf(__VA_ARGS__)

#define   ALOGD LOGD
#define   ALOGI LOGI
#define   ALOGW LOGW
#define   ALOGE LOGE
#endif  //LOGD

#ifndef nullptr
#define nullptr NULL
#endif

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif


#endif //UDPDEMO_LOG_H
