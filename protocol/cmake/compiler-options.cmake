if(MSVC) # Windows MSVC compiler base flags
  # Od makes it compile in debug mode in DEBUG or CI settings. Should give
  # better stack traces.  MT is for release, MTd is for debug, and for RELEASE
  # we should compile against the release version.
  add_compile_options(
    "$<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:-DWIN32;-DWIN32_LEAN_AND_MEAN;-DUNICODE;-D_CRT_SECURE_NO_WARNINGS;/W4;/wd4189;/wd4100;/wd4127;/wd4200;/wd4244;/MP;/WX;/EHsc;$<$<CONFIG:DEBUG>:/MTd;/Od>;$<$<CONFIG:RELEASE>:/MT;/O2>>"
  )
else() # GCC and Clang base flags
  add_compile_options(
    "-Wall"
    "-Wextra"
    "-Werror"
    # "-Wunreachable-code"
    "-Wno-missing-braces"
    "-Wno-unused-value"
    "-Wno-unused-parameter"
    "-Wno-unused-result"
    "-Wno-unused-variable" # Maybe we should bring this back?
    "-Wno-parentheses"
    "-Wno-missing-field-initializers"
    "-fexceptions" # Lets C++ exceptions pass through C code cleanly. MSVC already works (/EHc would turn it off).
    "-Wno-implicit-fallthrough" # We use switch/case fallthrough intentionally a lot, it should be allowed
    "-fno-common" # Error when two global variables have the same name, which would overlap them
    "-Wshadow" # Warn when a variable gets shadowed
    "$<$<COMPILE_LANGUAGE:C>:-Wincompatible-pointer-types>"
    "$<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>" # Warn when a function is declared as having unknown arguments.
    "$<$<COMPILE_LANGUAGE:C>:-Wmissing-prototypes>" # Warn when a global function has no prototype.
    "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,C>:-Werror=implicit-function-declaration>" # Error on implicit function declaration with C
    "$<$<CONFIG:DEBUG>:-Og;-g3;-O0>"
    "$<$<CONFIG:RELEASE>:-g3;-O2>"
    "$<$<BOOL:${CLIENT_SHARED_LIB}>:-fPIC>")
  add_link_options("-pthread" "-rdynamic")

  if(NOT "${SANITIZE}" STREQUAL "OFF")
    string(REPLACE "+" ";" SANITIZER_LIST ${SANITIZE})
    foreach(TYPE ${SANITIZER_LIST})
      add_compile_options("-fsanitize=${TYPE}")
      add_link_options("-fsanitize=${TYPE}")
    endforeach()
    if(CHECK_CI)
      add_compile_options("-fno-sanitize-recover=all")
      add_link_options("-fno-sanitize-recover=all")
    endif()
  endif()
endif()

# For now, log everything. For Metrics builds, also log CPU usage.
add_compile_definitions(LOG_LEVEL=5
                        $<$<CONFIG:Metrics>:LOG_CPU_USAGE=1>
                        __ROOT_FILE__="${PROJECT_SOURCE_DIR}")
