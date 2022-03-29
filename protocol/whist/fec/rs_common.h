
#pragma once

// the size of GF^8, which is the field size almost all RS libs use. (except FFT based RS)
#define RS_FIELD_SIZE 256

// temp workaround of for cm256 lib's CPU dispatching
// TODO: fix cm256
#define WHIST_FEC_ERROR_AVX2_NOT_SUPPORTED -100
