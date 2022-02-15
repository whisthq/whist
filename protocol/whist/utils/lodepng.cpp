/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file lodepng.cpp
 * @brief This file is a wrapper so that wrapped LodePNG
 *        unit tests can interpret `lodepng.c` as a C++ file
 */

// When linking the tests, certain globals may be multiply defined between the C and C++
// binaries (C comes from libWhistUtils, C++ directly for the LodePNG unit test). This
// causes the linker to fail. To fix this, we prevent redefinition of those globals in
// the C++ interpretation. This is not wanted/needed on MSVC due to symbol mangling in
// that case.
#ifndef _WIN32
#define WHIST_LODEPNG_CPP_SKIP_DUPLICATE_GLOBALS
#endif  // _WIN32
#define LODEPNG_MAX_ALLOC 100000000
#include "lodepng.c"
