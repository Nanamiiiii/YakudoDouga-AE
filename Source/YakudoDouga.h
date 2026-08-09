#pragma once

#ifndef YAKUDODOUGA_H
#define YAKUDODOUGA_H

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned short u_int16;
typedef unsigned long u_long;
typedef short int int16;

#define PF_TABLE_BITS 12
#define PF_TABLE_SZ_16 4096
#define PF_DEEP_COLOR_AWARE 1

#include "AEConfig.h"

#ifdef AE_OS_WIN
    typedef unsigned short PixelType;
    #include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include "YakudoDouga_Strings.h"

#define MAJOR_VERSION 1
#define MINOR_VERSION 0
#define BUG_VERSION 0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 9

#define YAKUDODOUGA_BLUR_TYPE_CHOICES 6
#define YAKUDODOUGA_BLUR_TYPE_DFLT 2

#define YAKUDODOUGA_CENTER_X_DFLT 50.0
#define YAKUDODOUGA_CENTER_Y_DFLT 50.0
#define YAKUDODOUGA_RESTRICT_BOUNDS 0

#define YAKUDODOUGA_BLUR_STRENGTH_MIN 0.0
#define YAKUDODOUGA_BLUR_STRENGTH_MAX 240.0
#define YAKUDODOUGA_BLUR_STRENGTH_DFLT 80.0

#define YAKUDODOUGA_MOTION_MULTIPLIER_MIN 0.0
#define YAKUDODOUGA_MOTION_MULTIPLIER_MAX 5.0
#define YAKUDODOUGA_MOTION_MULTIPLIER_DFLT 1.0

#define YAKUDODOUGA_MOTION_FRAME_COUNT_MIN 1
#define YAKUDODOUGA_MOTION_FRAME_COUNT_MAX 8
#define YAKUDODOUGA_MOTION_FRAME_COUNT_DFLT 1

#define BLUR_STRENGTH_DISK_ID 2
#define MOTION_MULTIPLIER_DISK_ID 1
#define MOTION_FRAME_COUNT_DISK_ID 3
#define BLUR_TYPE_DISK_ID 4
#define GLOBAL_MOTION_DISK_ID 5
#define CENTER_DISK_ID 6

enum {
    YAKUDODOUGA_BLUR_TYPE_LINEAR = 1,
    YAKUDODOUGA_BLUR_TYPE_GAUSSIAN,
    YAKUDODOUGA_BLUR_TYPE_OUTWARD,
    YAKUDODOUGA_BLUR_TYPE_JITTERED_GAUSSIAN,
    YAKUDODOUGA_BLUR_TYPE_MULTI_PASS,
    YAKUDODOUGA_BLUR_TYPE_DOWNSAMPLED
};

enum {
    YAKUDODOUGA_INPUT = 0,
    YAKUDODOUGA_CENTER,
    YAKUDODOUGA_BLUR_STRENGTH,
    YAKUDODOUGA_MOTION_MULTIPLIER,
    YAKUDODOUGA_MOTION_FRAME_COUNT,
    YAKUDODOUGA_BLUR_TYPE,
    YAKUDODOUGA_GLOBAL_MOTION,
    YAKUDODOUGA_NUM_PARAMS
};

extern "C" {
    DllExport
    PF_Err EffectMain(
        PF_Cmd cmd,
        PF_InData *in_data,
        PF_OutData *out_data,
        PF_ParamDef *params[],
        PF_LayerDef *output,
        void *extra);
}

#endif // YAKUDODOUGA_H
