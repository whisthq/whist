#include <whist/core/whist.h>
#ifndef WHIST_CORE_WHIST_MLOCK_H
#define WHIST_CORE_WHIST_MLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

void init_whist_mlock(void);

void whist_mlock(void* addr, size_t size);

void whist_munlock(void* addr);
#ifdef __cplusplus
};
#endif
#endif
