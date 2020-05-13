set(CMAKE_C_STANDARD 99)

include_directories(.)

add_executable(FractalServer
        main.c)

target_link_libraries(FractalServer STATIC
        PkgConfig::LIBAV
        )

#target_link_libraries(FractalServer SHARED
#        libvcruntime
#        libucrt
#        libcmt
#        kernel32
#        gdi32
#        winmm
#        imm32
#        shell32
#        advapi32
#        ole32
#        oleaut32
#        opengl32
#        user32
#        uuid
#        version
#        ws2_32
#        shlwapi
#        crypt32
#        d3d11
#        dxgi
#        shcore
#        )