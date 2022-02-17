add_subdirectory(cm256)

add_library(whistFEC STATIC
	rs.c
	fec.c
        rs_wrapper.c
        )

target_link_libraries(whistFEC whistFEC_cm256)

set_property(TARGET whistFEC PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
