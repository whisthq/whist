# #[[
################## Decoder Test Program ##################
#]]

set(DECODER_TEST_BINARY WhistDecoderTest)

add_executable(${DECODER_TEST_BINARY} decoder.c)
target_link_libraries(${DECODER_TEST_BINARY}
    ${PLATFORM_INDEPENDENT_LIBS})

copy_runtime_libs(${DECODER_TEST_BINARY})

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
