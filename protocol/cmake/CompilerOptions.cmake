if(MSVC) # Windows MSVC compiler base flags
  # Od makes it compile in debug mode in DEBUG or CI settings. Should give
  # better stack traces.  MT is for release, MTd is for debug, and for RELEASE
  # we should compile against the release version.
  add_compile_options(
    "$<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-DWIN32;-DWIN32_LEAN_AND_MEAN;-DUNICODE;/W4;/wd4189;/wd4100;/wd4127;/wd4244;/MP;$<$<BOOL:${CI}>:/WX;>;$<$<CONFIG:DEBUG>:/MTd;/Od>;$<$<CONFIG:RELEASE>:/MT;/O2>>"
  )
else() # GCC and Clang base flags
  add_compile_options(
    "-Wall"
    "-Wextra"
    "-Wno-missing-braces"
    "-Wno-unused-value"
    "-Wno-unused-parameter"
    "-Wno-unused-result"
    "-Wno-unused-variable"
    "-Wno-missing-field-initializers"
    "-Wno-implicit-fallthrough" # We use switch/case fallthrough intentionally a lot, it should be allowed
    "-fno-common" # Error when two global variables have the same name, which would overlap them
    "$<$<COMPILE_LANGUAGE:C>:-Wincompatible-pointer-types>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,C>:-Werror=implicit-function-declaration>" # Error on implicit function declaration with C
    "$<$<BOOL:${CI}>:-Werror>"
    "$<$<CONFIG:DEBUG>:-Og;-g;-O0>"
    "$<$<CONFIG:RELEASE>:-O3>")
  add_link_options("-pthread" "-rdynamic")
endif()

# if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
#   add_compile_options("-working-directory=/")
# endif()
