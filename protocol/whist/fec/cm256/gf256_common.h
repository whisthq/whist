#pragma once

#include <stdint.h> // uint32_t etc
#include <string.h> // memcpy, memset

/// Library header version
#define GF256_VERSION 2

//------------------------------------------------------------------------------
// Platform/Architecture

// WHIST_CHANGE
#if defined(__ARM_ARCH) || defined(__ARM_NEON) || defined(__ARM_NEON__)
    #if !defined IOS
        #if defined(__APPLE__)
            #define MACOS_ARM
        #else
            #define LINUX_ARM
        #endif
    #endif
#endif

// WHIST_CHANGE
#if defined(ANDROID) || defined(IOS) || defined(LINUX_ARM) || defined(__powerpc__) || defined(__s390__) || defined(MACOS_ARM)
    #define GF256_TARGET_MOBILE
#endif // ANDROID


#if !defined(GF256_TARGET_MOBILE)
    #define GF256_ALIGN_BYTES 32
#else
    #define GF256_ALIGN_BYTES 16
#endif

#if !defined(GF256_TARGET_MOBILE)
    #include <immintrin.h> // AVX2
    #include <tmmintrin.h> // SSSE3: _mm_shuffle_epi8
    #include <emmintrin.h> // SSE2
#else // GF256_TARGET_MOBILE
    #include <arm_neon.h>
#endif

// Compiler-specific 128-bit SIMD register keyword
#if defined(GF256_TARGET_MOBILE)
    #define GF256_M128 uint8x16_t
#else // GF256_TARGET_MOBILE
    #define GF256_M128 __m128i
#endif // GF256_TARGET_MOBILE

// Compiler-specific 256-bit SIMD register keyword
#if !defined(GF256_TARGET_MOBILE)
    #define GF256_M256 __m256i
#endif

// Compiler-specific C++11 restrict keyword
#define GF256_RESTRICT __restrict

// Compiler-specific force inline keyword
#ifdef _MSC_VER
    #define GF256_FORCE_INLINE inline __forceinline
#else
    #define GF256_FORCE_INLINE inline __attribute__((always_inline))
#endif

// Compiler-specific alignment keyword
// Note: Alignment only matters for ARM NEON where it should be 16
#ifdef _MSC_VER
    #define GF256_ALIGNED __declspec(align(GF256_ALIGN_BYTES))
#else // _MSC_VER
    #define GF256_ALIGNED __attribute__((aligned(GF256_ALIGN_BYTES)))
#endif // _MSC_VER

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


//------------------------------------------------------------------------------
// Portability

/// Swap two memory buffers in-place
extern void gf256_memswap(void * GF256_RESTRICT vx, void * GF256_RESTRICT vy, int bytes);


//------------------------------------------------------------------------------
// GF(256) Context

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4324) // warning C4324: 'gf256_ctx' : structure was padded due to __declspec(align())
#endif // _MSC_VER

/// The context object stores tables required to perform library calculations
typedef struct
{
    /// We require memory to be aligned since the SIMD instructions benefit from
    /// or require aligned accesses to the table data.
    struct
    {
        GF256_ALIGNED GF256_M128 TABLE_LO_Y[256];
        GF256_ALIGNED GF256_M128 TABLE_HI_Y[256];
    } MM128;
    struct
    {
        GF256_ALIGNED GF256_M256 TABLE_LO_Y[256];
        GF256_ALIGNED GF256_M256 TABLE_HI_Y[256];
    } MM256;

    /// Mul/Div/Inv/Sqr tables
    uint8_t GF256_MUL_TABLE[256 * 256];
    uint8_t GF256_DIV_TABLE[256 * 256];
    uint8_t GF256_INV_TABLE[256];
    uint8_t GF256_SQR_TABLE[256];

    /// Log/Exp tables
    uint16_t GF256_LOG_TABLE[256];
    uint8_t GF256_EXP_TABLE[512 * 2 + 1];

    /// Polynomial used
    unsigned Polynomial;
} gf256_ctx;

#ifdef _MSC_VER
    #pragma warning(pop)
#endif // _MSC_VER

extern gf256_ctx GF256Ctx;

#ifdef __cplusplus
}
#endif // __cplusplus
