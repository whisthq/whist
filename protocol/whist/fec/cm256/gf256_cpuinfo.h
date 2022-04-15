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


// convert CpuType to str
inline const char * cpu_type_to_str(CpuType cpu_type) {
    switch (cpu_type) {
        case CPU_TYPE_X86: {
            return "x86";
        }
        case CPU_TYPE_X64: {
            return "x64";
        }
        case CPU_TYPE_ARM32: {
            return "arm32";
        }
        case CPU_TYPE_ARM64: {
            return "arm64";
        }
        case CPU_TYPE_OTHER: {
            return "other";
        }
        default: {
            return "invalid";
        }
    }
}

// get the cpu type and intruction support info
CpuInfo gf256_get_cpuinfo(void);

#ifdef __cplusplus
}
#endif
