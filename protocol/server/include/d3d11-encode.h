 #ifdef __cplusplus
 #define EXTERNC extern "C"
 #else
 #define EXTERNC
 #endif

 EXTERNC int hello_world();

 #undef EXTERNC