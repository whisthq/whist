if (MSVC) #Windows MSVC compiler base flags
    set(CMAKE_C_FLAGS_INIT "-DWIN32 -DWIN32_LEAN_AND_MEAN -DUNICODE")
    set(CMAKE_C_FLAGS_DEBUG_INIT "/W4 /MP /MTd /O2i" )
    set(CMAKE_C_FLAGS_RELEASE_INIT "/W4 /MP /MTd /O2i" )
    set(CMAKE_C_FLAGS_CI_INIT "/W4 /WX /MP /MTd /O2i" )
else() #GCC and Clang base flags
    set(CMAKE_C_FLAGS_INIT "")
    set(CMAKE_C_FLAGS_DEBUG_INIT "-g -Wall -Wextra -Wno-missing-braces -Wno-unused-value -Wno-unused-parameter -O0 -pthread")
    set(CMAKE_C_FLAGS_RELEASE_INIT "-g -Wall -Wextra -Wno-missing-braces -Wno-unused-value -Wno-unused-parameter -O3 -pthread")
    set(CMAKE_C_FLAGS_CI_INIT "-g -Wall -Wextra -Wno-missing-braces -Wno-unused-value -Werror -Wno-unused-parameter -O0 -pthread")
endif()