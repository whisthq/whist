add_compile_options("-mavx2")

add_library(whistFEC_cm256_avx2 STATIC gf256_avx2.cpp)
