# vim: syntax=cmake
# hal jpeg reg

set(HAL_JPEGD_HDR
    hal_jpegd_reg.h
    )

set(HAL_JPEGD_SRC
    hal_jpegd_reg.c
    )

add_library(hal_jpegd STATIC
    ${HAL_JPEGD_SRC} ${HAL_JPEGD_HDR}
    )

set_target_properties(hal_jpegd PROPERTIES FOLDER "mpp/hal")

    target_link_libraries(hal_jpegd mpp_base)

#add_subdirectory(test)
