#pragma once
#include <stdint.h>
#include <udi/udi.h>

#ifdef __cplusplus
extern "C" {
#endif
uint64_t DEV_LoadUDIDriver();
uint64_t DEV_InitUDIDriver();
uint64_t DEV_InitUDIInstance();
uint64_t DEV_InitUDIRegion();
#ifdef __cplusplus
}
#endif

