if (MSVC)
    set(CMAKE_C_FLAGS_INIT "-DWIN32")
    set(CMAKE_C_FLAGS_DEBUG_INIT "-DWIN32_LEAN_AND_MEAN -DUNICODE /W4 /MP /MTd /O2i" )
    set(CMAKE_C_FLAGS_RELEASE_INIT "-DWIN32_LEAN_AND_MEAN -DUNICODE /W4 /MP /MTd /O2i" )
endif()