add_library(fractalNetwork STATIC
        network.c
        network.h)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	target_link_libraries(fractalNetwork curl)
else()
	target_link_libraries(fractalNetwork winhttp)
endif()

set_property(TARGET fractalNetwork PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
