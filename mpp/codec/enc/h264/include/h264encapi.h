/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __H264ENCAPI_H__
#define __H264ENCAPI_H__

#include "rk_mpi.h"

#include "h264_syntax.h"
#include "h264e_syntax.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Function return values */
typedef enum {
    H264ENC_OK = 0,
    H264ENC_FRAME_READY = 1,

    H264ENC_ERROR = -1,
    H264ENC_NULL_ARGUMENT = -2,
    H264ENC_INVALID_ARGUMENT = -3,
    H264ENC_MEMORY_ERROR = -4,
    H264ENC_EWL_ERROR = -5,
    H264ENC_EWL_MEMORY_ERROR = -6,
    H264ENC_INVALID_STATUS = -7,
    H264ENC_OUTPUT_BUFFER_OVERFLOW = -8,
    H264ENC_HW_BUS_ERROR = -9,
    H264ENC_HW_DATA_ERROR = -10,
    H264ENC_HW_TIMEOUT = -11,
    H264ENC_HW_RESERVED = -12,
    H264ENC_SYSTEM_ERROR = -13,
    H264ENC_INSTANCE_ERROR = -14,
    H264ENC_HRD_ERROR = -15,
    H264ENC_HW_RESET = -16
} H264EncRet;

/* Picture rotation for pre-processing */
typedef enum {
    H264ENC_ROTATE_0 = 0,
    H264ENC_ROTATE_90R = 1, /* Rotate 90 degrees clockwise */
    H264ENC_ROTATE_90L = 2  /* Rotate 90 degrees counter-clockwise */
} H264EncPictureRotation;

/* Picture color space conversion (RGB input) for pre-processing */
typedef enum {
    H264ENC_RGBTOYUV_BT601 = 0, /* Color conversion according to BT.601 */
    H264ENC_RGBTOYUV_BT709 = 1, /* Color conversion according to BT.709 */
    H264ENC_RGBTOYUV_USER_DEFINED = 2   /* User defined color conversion */
} H264EncColorConversionType;

enum H264EncStatus {
    H264ENCSTAT_INIT = 0xA1,
    H264ENCSTAT_START_STREAM,
    H264ENCSTAT_START_FRAME,
    H264ENCSTAT_ERROR
};

/* Stream type for initialization */
typedef enum {
    H264ENC_BYTE_STREAM = 0,    /* H.264 annex B: NAL unit starts with
                                     * hex bytes '00 00 00 01' */
    H264ENC_NAL_UNIT_STREAM = 1 /* Plain NAL units without startcode */
} H264EncStreamType;

/* Picture type for encoding */
typedef enum {
    H264ENC_INTRA_FRAME = 0,
    H264ENC_PREDICTED_FRAME = 1,
    H264ENC_NOTCODED_FRAME  /* Used just as a return value */
} H264EncPictureCodingType;

/* Configuration info for initialization
 * Width and height are picture dimensions after rotation
 * Width and height are restricted by level limitations
 */
typedef struct {
    H264EncStreamType streamType;   /* Byte stream / Plain NAL units */
    /* Stream Profile will be automatically decided,
     * CABAC -> main/high, 8x8-transform -> high */
    H264Profile profile;
    H264Level   level;
    RK_U32 width;           /* Encoded picture width in pixels, multiple of 4 */
    RK_U32 height;          /* Encoded picture height in pixels, multiple of 2 */
    RK_U32 hor_stride;      /* Encoded picture horizontal stride in pixels */
    RK_U32 ver_stride;      /* Encoded picture vertical stride in pixels */
    RK_U32 frameRateNum;    /* The stream time scale, [1..65535] */
    RK_U32 frameRateDenom;  /* Maximum frame rate is frameRateNum/frameRateDenom
                              * in frames/second. The actual frame rate will be
                              * defined by timeIncrement of encoded pictures,
                              * [1..frameRateNum] */
    RK_U32 enable_cabac;
    RK_U32 cabac_idc;
    RK_U32 transform8x8_mode;
    RK_U32 pic_init_qp;
    RK_U32 chroma_qp_index_offset;
    RK_U32 pic_luma_height;
    RK_U32 pic_luma_width;
    MppFrameFormat input_image_format;
    RK_U32 second_chroma_qp_index_offset;
    RK_U32 pps_id;
} H264EncConfig;


#define H264E_DBG_FUNCTION          (0x00000001)

extern RK_U32 h264e_debug;

#define h264e_dbg(flag, fmt, ...)   _mpp_dbg(h264e_debug, flag, fmt, ## __VA_ARGS__)
#define h264e_dbg_f(flag, fmt, ...) _mpp_dbg_f(h264e_debug, flag, fmt, ## __VA_ARGS__)

#define h264e_dbg_func(fmt, ...)    h264e_dbg_f(H264E_DBG_FUNCTION, fmt, ## __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /*__H264ENCAPI_H__*/
