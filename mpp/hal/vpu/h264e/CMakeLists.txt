# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h264/)

# hal h264 header
set(HAL_H264E_HDR
    hal_h264e_vpu.h
    )

# hal h264 encoder sourse
set(HAL_H264E_SRC
    hal_h264e_vpu.c
    )

add_library(hal_h264e_vpu STATIC
            ${HAL_H264E_HDR}
            ${HAL_H264E_SRC}
            )
if(RKPLATFORM)
    target_link_libraries(hal_h264e_vpu hal_h264e_com worker_vpu mpp_base)
else()
    target_link_libraries(hal_h264e_vpu hal_h264e_com mpp_base)
endif()

set_target_properties(hal_h264e_vpu PROPERTIES FOLDER "mpp/hal")
