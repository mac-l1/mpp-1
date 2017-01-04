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

#define MODULE_TAG "h264e_api"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "mpp_rc.h"

#include "h264e_api.h"
#include "h264e_codec.h"
#include "h264e_syntax.h"

#include "mpp_controller.h"

#define H264E_DBG_FUNCTION          (0x00000001)

#define h264e_dbg(flag, fmt, ...)   _mpp_dbg(h264e_debug, flag, fmt, ## __VA_ARGS__)
#define h264e_dbg_f(flag, fmt, ...) _mpp_dbg_f(h264e_debug, flag, fmt, ## __VA_ARGS__)

#define h264e_dbg_func(fmt, ...)    h264e_dbg_f(H264E_DBG_FUNCTION, fmt, ## __VA_ARGS__)

RK_U32 h264e_debug = 0;

typedef struct {
    /* config from mpp_enc */
    MppEncCfgSet    *cfg;
    MppEncCfgSet    *set;
    RK_U32          idr_request;

    /* internal rate control config */
    RK_U32          rc_ready;
    RK_U32          prep_ready;
    MppRateControl  *rc;

    /* output to hal */
    RcSyntax        syntax;

    /*
     * input from hal
     * TODO: on link table mode there will be multiple result
     */
    RcHalResult     result;
} H264eCtx;

MPP_RET h264e_init(void *ctx, ControllerCfg *ctrl_cfg)
{
    MPP_RET ret = MPP_OK;
    H264eCtx *p = (H264eCtx *)ctx;
    MppEncRcCfg *rc_cfg = &ctrl_cfg->cfg->rc;
    MppEncPrepCfg *prep = &ctrl_cfg->cfg->prep;

    h264e_dbg_func("enter\n");

    /*
     * default prep:
     * 720p
     * YUV420SP
     */
    prep->change = 0;
    prep->width = 1280;
    prep->height = 720;
    prep->hor_stride = 1280;
    prep->ver_stride = 720;
    prep->format = MPP_FMT_YUV420SP;
    prep->rotation = 0;
    prep->mirroring = 0;
    prep->denoise = 0;

    /*
     * default rc_cfg:
     * CBR
     * 2Mbps +-25%
     * 30fps
     * gop 60
     */
    rc_cfg->change = 0;
    rc_cfg->rc_mode = 3;
    rc_cfg->bps_target = 2000 * 1000;
    rc_cfg->bps_max = rc_cfg->bps_target * 5 / 4;
    rc_cfg->bps_min = rc_cfg->bps_target * 3 / 4;
    rc_cfg->fps_in_flex = 0;
    rc_cfg->fps_in_num = 30;
    rc_cfg->fps_in_denorm = 1;
    rc_cfg->fps_out_flex = 0;
    rc_cfg->fps_out_num = 30;
    rc_cfg->fps_out_denorm = 1;
    rc_cfg->gop = 60;
    rc_cfg->skip_cnt = 0;

    p->cfg = ctrl_cfg->cfg;
    p->set = ctrl_cfg->set;
    p->idr_request = 0;

    ret = mpp_rc_init(&p->rc);

    mpp_env_get_u32("h264e_debug", &h264e_debug, 0);

    h264e_dbg_func("leave\n");
    return ret;
}

MPP_RET h264e_deinit(void *ctx)
{
    H264eCtx *p = (H264eCtx *)ctx;

    h264e_dbg_func("enter\n");

    if (p->rc)
        mpp_rc_deinit(p->rc);

    h264e_dbg_func("leave\n");
    return MPP_OK;
}

MPP_RET h264e_encode(void *ctx, HalEncTask *task)
{
    H264eCtx *p = (H264eCtx *)ctx;
    RcSyntax *rc_syn = &p->syntax;
    MppEncCfgSet *cfg = p->cfg;
    MppEncRcCfg *rc = &cfg->rc;

    if (!p->rc_ready) {
        mpp_err_f("not initialize encoding\n");
        task->valid = 0;
        return MPP_NOK;
    }

    mpp_rc_update_user_cfg(p->rc, rc);

    mpp_rc_bits_allocation(p->rc, rc_syn);

    task->syntax.data   = &p->syntax;
    task->syntax.number = 1;
    task->valid = 1;

    return MPP_OK;
}

MPP_RET h264e_reset(void *ctx)
{
    (void)ctx;
    return MPP_OK;
}

MPP_RET h264e_flush(void *ctx)
{
    (void)ctx;
    return MPP_OK;
}


static const H264Profile h264e_supported_profile[] = {
    H264_PROFILE_BASELINE,
    H264_PROFILE_MAIN,
    H264_PROFILE_HIGH,
};

static const H264Level h264e_supported_level[] = {
    H264_LEVEL_1_0,
    H264_LEVEL_1_b,
    H264_LEVEL_1_1,
    H264_LEVEL_1_2,
    H264_LEVEL_1_3,
    H264_LEVEL_2_0,
    H264_LEVEL_2_1,
    H264_LEVEL_2_2,
    H264_LEVEL_3_0,
    H264_LEVEL_3_1,
    H264_LEVEL_3_2,
    H264_LEVEL_4_0,
    H264_LEVEL_4_1,
    H264_LEVEL_4_2,
    H264_LEVEL_5_0,
    H264_LEVEL_5_1,
};

static MPP_RET h264e_check_mpp_cfg(MppEncConfig *mpp_cfg)
{
    RK_U32 i, count;

    count = MPP_ARRAY_ELEMS(h264e_supported_profile);
    for (i = 0; i < count; i++) {
        if (h264e_supported_profile[i] == (H264Profile)mpp_cfg->profile) {
            break;
        }
    }

    if (i >= count) {
        mpp_log_f("invalid profile %d set to default baseline\n", mpp_cfg->profile);
        mpp_cfg->profile = H264_PROFILE_BASELINE;
    }

    count = MPP_ARRAY_ELEMS(h264e_supported_level);
    for (i = 0; i < count; i++) {
        if (h264e_supported_level[i] == (H264Level)mpp_cfg->level) {
            break;
        }
    }

    if (i >= count) {
        mpp_log_f("invalid level %d set to default 4.0\n", mpp_cfg->level);
        mpp_cfg->level = H264_LEVEL_4_0;
    }

    if (mpp_cfg->fps_in <= 0) {
        mpp_log_f("invalid input fps %d will be set to default 30\n", mpp_cfg->fps_in);
        mpp_cfg->fps_in = 30;
    }

    if (mpp_cfg->fps_out <= 0) {
        mpp_log_f("invalid output fps %d will be set to fps_in 30\n", mpp_cfg->fps_out, mpp_cfg->fps_in);
        mpp_cfg->fps_out = mpp_cfg->fps_in;
    }

    if (mpp_cfg->gop <= 0) {
        mpp_log_f("invalid gop %d will be set to fps_out 30\n", mpp_cfg->gop, mpp_cfg->fps_out);
        mpp_cfg->gop = mpp_cfg->fps_out;
    }

    if (mpp_cfg->rc_mode < 0 || mpp_cfg->rc_mode > 2) {
        mpp_err_f("invalid rc_mode %d\n", mpp_cfg->rc_mode);
        return MPP_NOK;
    }

    if (mpp_cfg->qp <= 0 || mpp_cfg->qp > 51) {
        mpp_log_f("invalid qp %d set to default 26\n", mpp_cfg->qp);
        mpp_cfg->qp = 26;
    }

    if (mpp_cfg->bps <= 0) {
        mpp_err_f("invalid bit rate %d\n", mpp_cfg->bps);
        return MPP_NOK;
    }

    if (mpp_cfg->width <= 0 || mpp_cfg->height <= 0) {
        mpp_err_f("invalid width %d height %d\n", mpp_cfg->width, mpp_cfg->height);
        return MPP_NOK;
    }

    if (mpp_cfg->hor_stride <= 0) {
        mpp_err_f("invalid hor_stride %d will be set to %d\n", mpp_cfg->hor_stride, mpp_cfg->width);
        mpp_cfg->hor_stride = mpp_cfg->width;
    }

    if (mpp_cfg->ver_stride <= 0) {
        mpp_err_f("invalid ver_stride %d will be set to %d\n", mpp_cfg->ver_stride, mpp_cfg->height);
        mpp_cfg->ver_stride = mpp_cfg->height;
    }

    return MPP_OK;
}

MPP_RET h264e_config(void *ctx, RK_S32 cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    H264eCtx *p = (H264eCtx *)ctx;

    h264e_dbg_func("enter ctx %p cmd %x param %p\n", ctx, cmd, param);

    switch (cmd) {
    case CHK_ENC_CFG : {
        ret = h264e_check_mpp_cfg((MppEncConfig *)param);
    } break;
    case SET_IDR_FRAME : {
        p->idr_request++;
    } break;
    case MPP_ENC_SET_RC_CFG : {
        mpp_log_f("MPP_ENC_SET_RC_CFG bps %d [%d : %d]\n", p->set->rc.bps_target,
                  p->set->rc.bps_min, p->set->rc.bps_max);
        p->rc_ready = 1;
    } break;
    default:
        mpp_err("No correspond cmd found, and can not config!");
        ret = MPP_NOK;
        break;
    }

    h264e_dbg_func("leave ret %d\n", ret);

    return ret;
}

MPP_RET h264e_callback(void *ctx, void *feedback)
{
    H264eCtx *p = (H264eCtx *)ctx;
    h264e_feedback *fb  = (h264e_feedback *)feedback;

    p->result = *fb->result;
    mpp_rc_update_hw_result(p->rc, fb->result);
    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/
const ControlApi api_h264e_controller = {
    "h264e_control",
    MPP_VIDEO_CodingAVC,
    sizeof(H264eCtx),
    0,
    h264e_init,
    h264e_deinit,
    h264e_encode,
    h264e_reset,
    h264e_flush,
    h264e_config,
    h264e_callback,
};
