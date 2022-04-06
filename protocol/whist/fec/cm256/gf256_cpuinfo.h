#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CPU_TYPE_X86,
    CPU_TYPE_X64,
    CPU_TYPE_ARM32,
    CPU_TYPE_ARM64,
    CPU_TYPE_OTHER,
} CpuType;

typedef struct
{
    CpuType cpu_type;
    bool has_ssse3;
    bool has_avx2;
    bool has_neon;
} CpuInfo;
// get the cpu type and intruction support info
CpuInfo gf256_get_cpuinfo(void);

#ifdef __cplusplus
}
#endif
