# #[[
################## Decoder Test Program ##################
#]]

set(DECODER_TEST_BINARY WhistDecoderTest)

add_executable(${DECODER_TEST_BINARY} decoder.c)
target_link_libraries(${DECODER_TEST_BINARY}
    ${PLATFORM_INDEPENDENT_LIBS})

copy_runtime_libs(${DECODER_TEST_BINARY})

set(SDL2_DIR_PATH "${CMAKE_BINARY_DIR}/lib/${arch}/SDL2/${CMAKE_SYSTEM_NAME}")
find_library(STATIC_SDL2 NAMES SDL2 SDL2-static PATHS "${SDL2_DIR_PATH}"
        NO_DEFAULT_PATH
        NO_CMAKE_ENVIRONMENT_PATH
        NO_CMAKE_PATH
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_CMAKE_FIND_ROOT_PATH)
if(NOT STATIC_SDL2)
    message(FATAL_ERROR "Library STATIC_SDL2 was not found! ${SDL2_DIR_PATH}")
endif()
target_link_libraries(${DECODER_TEST_BINARY} ${STATIC_SDL2})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_EXE_LINKER_FLAGS "/nodefaultlib /nologo /ignore:4099  ")

    set_property(TARGET ${DECODER_TEST_BINARY} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )

    target_link_libraries(${DECODER_TEST_BINARY} ${WINDOWS_CORE_LIBS})
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(${DECODER_TEST_BINARY} ${MAC_SPECIFIC_CLIENT_LIBS})
else()
    target_link_libraries(${DECODER_TEST_BINARY} OpenSSL::Crypto)
endif()
