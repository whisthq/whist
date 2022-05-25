#ifndef WHIST_PLATFORM_H
#define WHIST_PLATFORM_H
/**
 * @copyright (c) 2022 Whist Technologies, Inc.
 * @file platform.h
 * @brief This file defines preprocessor macros for determining
 *        the current operating system and architecture.
 *
============================
Usage
============================

To run code only on Linux machines, use an if-block:
```
if (OS_IS(OS_LINUX)) {
    // Code only runs on Linux
}
```

To compile code only on Linux or macOS machines, use a preprocessor `#if`:
```
#if OS_IN(OS_LINUX | OS_MACOS)
    // Code only compiles on Linux or macOS
#endif
```

Similarly, to only run code on an x86_64 machine, use:
```
if (ARCH_IS(ARCH_X86_64)) {
    // Code only runs on x86_64
}
```

Likewise, for only compiling on arm64 machines, use:
```
#if ARCH_IS(ARCH_ARM_64)
    // Code only compiles on arm64
#endif
```
 */

/*
============================
Defines
============================
*/

#define OS_UNDEF 0x00  ///< An unknown or unspecified operating system
#define OS_WIN32 0x01  ///< A Windows machine
#define OS_LINUX 0x02  ///< A Linux machine
#define OS_MACOS 0x04  ///< A macOS machine

/**
 * @brief The current operating system
 */
#if defined(_WIN32)
#define CURRENT_OS OS_WIN32
#elif defined(__linux__)
#define CURRENT_OS OS_LINUX
#elif defined(__APPLE__)
#define CURRENT_OS OS_MACOS
#else
#define CURRENT_OS OS_UNDEF
#endif  // OS type

/**
 * @brief Returns whether the current operating system is the given type
 *
 * @param OS The operating system to check for
 *
 * @return `true` if the current operating system is the given type, else `false`
 */
#define OS_IS(OS) ((CURRENT_OS) == (OS))

/**
 * @brief Returns whether the current operating system one of the given types
 *
 * @param OS A bitwise OR (`|`) of the operating systems to check for
 *
 * @return `true` if the current operating system in the given types, else `false`
 */
#define OS_IN(OS) (((CURRENT_OS) & (OS)) != 0)

#define ARCH_UNDEF 0x00  ///< An unknown or unspecified architecture
#define ARCH_X86 0x01    ///< An x86 machine
#define ARCH_ARM 0x02    ///< An ARM machine
#define ARCH_32 0x04     ///< A 32-bit architecture
#define ARCH_64 0x08     ///< A 64-bit architecture

#define ARCH_X86_64 (ARCH_X86 | ARCH_64)  ///< An x86_64 machine
#define ARCH_X86_32 (ARCH_X86 | ARCH_32)  ///< An x86 32-bit machine
#define ARCH_ARM_64 (ARCH_ARM | ARCH_64)  ///< An arm64 machine
#define ARCH_ARM_32 (ARCH_ARM | ARCH_32)) ///< An arm32 machine

/**
 * @brief The current system architecture
 */
// TODO: Reconcile this with the CPU detection in gf256.cpp and corresponding headers.
#if defined(__x86_64__)
#define CURRENT_ARCH ARCH_X86 | ARCH_64
#elif defined(__aarch64__) || defined(__arm64__)
#define CURRENT_ARCH ARCH_ARM | ARCH_64
#else
#define CURRENT_ARCH ARCH_UNDEF
#endif  // arch type

/**
 * @brief Returns whether the current system architecture is the given type
 *
 * @param ARCH The architecture to check for
 *
 * @return `true` if the current architecture is the given type, else `false`
 */
#define ARCH_IS(ARCH) ((CURRENT_ARCH) == (ARCH))

#endif  // WHIST_PLATFORM_H
