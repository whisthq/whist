#pragma once

// temp solution to expose the CPU check out function to upper level
// the final solution will be clearer after we fixed the sse2 support for cm256 lib
bool gf256_has_hardware_support(void);
