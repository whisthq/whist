#pragma once
/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file gf256_cpuinfo.h
 * @brief This file contains functions that exposes the gf256 cpu detection to outside
 */

#ifdef __cplusplus
extern "C" {
#endif

// the enum that defines cpu types
typedef enum
{
    CPU_TYPE_X86,
    CPU_TYPE_X64,
    CPU_TYPE_ARM32,
    CPU_TYPE_ARM64,
    CPU_TYPE_OTHER,
} CpuType;

// the struct containing cpu type and instruction support info
typedef struct
{
    CpuType cpu_type;
    bool has_sse2;
    bool has_ssse3;
    bool has_avx2;
    bool has_neon;
} CpuInfo;

/**
 * @brief                          get the cpu type and intruction support info
 * @returns                        the detected infos
 */
CpuInfo gf256_get_cpuinfo(void);

/**
 * @brief                          convert CpuType to str
 * @param cpu_type                 the cpu_type
 * @returns                        the pointer to str
 */
const char * cpu_type_to_str(CpuType cpu_type);

#ifdef __cplusplus
}
#endif
