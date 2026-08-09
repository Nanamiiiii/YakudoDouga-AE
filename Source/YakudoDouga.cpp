#include "YakudoDouga.h"

#include <cmath>

namespace {

constexpr A_long kMaxSampleCount = 33;

static A_long ClampLong(
    A_long value,
    A_long minimum,
    A_long maximum)
{
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

static float ClampFloat(
    float value,
    float minimum,
    float maximum)
{
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

template <typename Pixel>
static const Pixel *PixelAt(
    const PF_LayerDef &world,
    A_long x,
    A_long y)
{
    const char *row = reinterpret_cast<const char *>(world.data) + (y * world.rowbytes);
    return reinterpret_cast<const Pixel *>(row) + x;
}

template <typename Pixel>
static Pixel *PixelAt(
    PF_LayerDef &world,
    A_long x,
    A_long y)
{
    char *row = reinterpret_cast<char *>(world.data) + (y * world.rowbytes);
    return reinterpret_cast<Pixel *>(row) + x;
}

template <typename Pixel>
static float MotionAmount(
    const Pixel &current,
    const Pixel &previous,
    float maxChannelValue)
{
    const float redDifference = std::fabs(static_cast<float>(current.red) - static_cast<float>(previous.red));
    const float greenDifference = std::fabs(static_cast<float>(current.green) - static_cast<float>(previous.green));
    const float blueDifference = std::fabs(static_cast<float>(current.blue) - static_cast<float>(previous.blue));

    return ((redDifference + greenDifference + blueDifference) / 3.0f) / maxChannelValue;
}

static float StableJitter(
    A_long x,
    A_long y,
    A_long sampleIndex)
{
    A_u_long seed = static_cast<A_u_long>(x) * 73856093u;
    seed ^= static_cast<A_u_long>(y) * 19349663u;
    seed ^= static_cast<A_u_long>(sampleIndex) * 83492791u;
    seed ^= seed >> 13;
    seed *= 1274126177u;

    return (static_cast<float>(seed & 1023u) / 1023.0f) - 0.5f;
}

static float GaussianWeight(float offsetRatio)
{
    constexpr float sigma = 0.45f;
    const float normalizedOffset = offsetRatio / sigma;

    return std::exp(-0.5f * normalizedOffset * normalizedOffset);
}

static float SampleOffsetRatio(
    A_long blurType,
    A_long sampleIndex,
    A_long sampleCount,
    A_long x,
    A_long y)
{
    if (sampleCount <= 1) {
        return 0.0f;
    }

    if (blurType == YAKUDODOUGA_BLUR_TYPE_OUTWARD) {
        return static_cast<float>(sampleIndex) / static_cast<float>(sampleCount - 1);
    }

    const float halfSampleSpan = static_cast<float>(sampleCount - 1) * 0.5f;
    float offsetRatio = (static_cast<float>(sampleIndex) - halfSampleSpan) / halfSampleSpan;

    if (blurType == YAKUDODOUGA_BLUR_TYPE_JITTERED_GAUSSIAN) {
        offsetRatio += StableJitter(x, y, sampleIndex) / halfSampleSpan;
        offsetRatio = ClampFloat(offsetRatio, -1.0f, 1.0f);
    }

    return offsetRatio;
}

static float SampleWeight(
    A_long blurType,
    float offsetRatio)
{
    switch (blurType) {
        case YAKUDODOUGA_BLUR_TYPE_GAUSSIAN:
        case YAKUDODOUGA_BLUR_TYPE_JITTERED_GAUSSIAN:
        case YAKUDODOUGA_BLUR_TYPE_DOWNSAMPLED:
            return GaussianWeight(offsetRatio);

        case YAKUDODOUGA_BLUR_TYPE_MULTI_PASS: {
            const float tentWeight = 1.0f - std::fabs(offsetRatio);
            return tentWeight * tentWeight;
        }

        case YAKUDODOUGA_BLUR_TYPE_OUTWARD:
            return std::exp(-2.0f * offsetRatio * offsetRatio);

        case YAKUDODOUGA_BLUR_TYPE_LINEAR:
        default:
            return 1.0f;
    }
}

template <typename Pixel>
static void AccumulatePixel(
    const Pixel &pixel,
    float sampleWeight,
    float &alpha,
    float &red,
    float &green,
    float &blue,
    float &weightTotal)
{
    alpha += static_cast<float>(pixel.alpha) * sampleWeight;
    red += static_cast<float>(pixel.red) * sampleWeight;
    green += static_cast<float>(pixel.green) * sampleWeight;
    blue += static_cast<float>(pixel.blue) * sampleWeight;
    weightTotal += sampleWeight;
}

template <typename Pixel>
static void AccumulatePointSample(
    const PF_LayerDef &input,
    A_long sampleX,
    A_long sampleY,
    float sampleWeight,
    float &alpha,
    float &red,
    float &green,
    float &blue,
    float &weightTotal)
{
    const Pixel *sample = PixelAt<Pixel>(input, sampleX, sampleY);
    AccumulatePixel(*sample, sampleWeight, alpha, red, green, blue, weightTotal);
}

template <typename Pixel>
static void AccumulateBoxSample(
    const PF_LayerDef &input,
    A_long sampleX,
    A_long sampleY,
    A_long radius,
    float sampleWeight,
    float &alpha,
    float &red,
    float &green,
    float &blue,
    float &weightTotal)
{
    float boxAlpha = 0.0f;
    float boxRed = 0.0f;
    float boxGreen = 0.0f;
    float boxBlue = 0.0f;
    A_long boxCount = 0;

    for (A_long boxY = sampleY - radius; boxY <= sampleY + radius; ++boxY) {
        const A_long clampedY = ClampLong(boxY, 0, input.height - 1);

        for (A_long boxX = sampleX - radius; boxX <= sampleX + radius; ++boxX) {
            const A_long clampedX = ClampLong(boxX, 0, input.width - 1);
            const Pixel *sample = PixelAt<Pixel>(input, clampedX, clampedY);

            boxAlpha += static_cast<float>(sample->alpha);
            boxRed += static_cast<float>(sample->red);
            boxGreen += static_cast<float>(sample->green);
            boxBlue += static_cast<float>(sample->blue);
            ++boxCount;
        }
    }

    if (boxCount <= 0) {
        return;
    }

    alpha += (boxAlpha / static_cast<float>(boxCount)) * sampleWeight;
    red += (boxRed / static_cast<float>(boxCount)) * sampleWeight;
    green += (boxGreen / static_cast<float>(boxCount)) * sampleWeight;
    blue += (boxBlue / static_cast<float>(boxCount)) * sampleWeight;
    weightTotal += sampleWeight;
}

template <typename Pixel>
static float ComputeGlobalMotionRatio(
    const PF_LayerDef &input,
    const PF_LayerDef *const previousInputs[],
    A_long previousInputCount,
    float motionMultiplier,
    float maxChannelValue)
{
    if (input.width <= 0 || input.height <= 0) {
        return 0.0f;
    }

    double motionTotal = 0.0;
    A_long sampleTotal = 0;

    for (A_long y = 0; y < input.height; ++y) {
        for (A_long x = 0; x < input.width; ++x) {
            const Pixel *currentPixel = PixelAt<Pixel>(input, x, y);

            for (A_long frameIndex = 0; frameIndex < previousInputCount; ++frameIndex) {
                const PF_LayerDef *previousInput = previousInputs[frameIndex];

                if (previousInput && previousInput->data && previousInput->width > 0 && previousInput->height > 0) {
                    const A_long previousX = ClampLong(x, 0, previousInput->width - 1);
                    const A_long previousY = ClampLong(y, 0, previousInput->height - 1);
                    const Pixel *previousPixel = PixelAt<Pixel>(*previousInput, previousX, previousY);

                    motionTotal += MotionAmount(*currentPixel, *previousPixel, maxChannelValue);
                    ++sampleTotal;
                }
            }
        }
    }

    if (sampleTotal <= 0) {
        return 0.0f;
    }

    return ClampFloat(
        static_cast<float>(motionTotal / static_cast<double>(sampleTotal)) * motionMultiplier,
        0.0f,
        1.0f);
}

template <typename Pixel>
static void RenderMotionRadialBlurTyped(
    const PF_LayerDef &input,
    const PF_LayerDef *const previousInputs[],
    A_long previousInputCount,
    PF_LayerDef &output,
    float centerX,
    float centerY,
    float blurStrength,
    float motionMultiplier,
    A_long blurType,
    PF_Boolean useGlobalMotion,
    float maxChannelValue)
{
    const A_long width = output.width;
    const A_long height = output.height;

    if (width <= 0 || height <= 0) {
        return;
    }

    const float clampedCenterX = ClampFloat(centerX, 0.0f, static_cast<float>(width) - 1.0f);
    const float clampedCenterY = ClampFloat(centerY, 0.0f, static_cast<float>(height) - 1.0f);
    const float rightDistance = static_cast<float>(width) - 1.0f - clampedCenterX;
    const float bottomDistance = static_cast<float>(height) - 1.0f - clampedCenterY;
    const float topLeftDistance = std::sqrt((clampedCenterX * clampedCenterX) + (clampedCenterY * clampedCenterY));
    const float topRightDistance = std::sqrt((rightDistance * rightDistance) + (clampedCenterY * clampedCenterY));
    const float bottomLeftDistance = std::sqrt((clampedCenterX * clampedCenterX) + (bottomDistance * bottomDistance));
    const float bottomRightDistance = std::sqrt((rightDistance * rightDistance) + (bottomDistance * bottomDistance));
    float maxDistance = topLeftDistance;

    if (topRightDistance > maxDistance) {
        maxDistance = topRightDistance;
    }

    if (bottomLeftDistance > maxDistance) {
        maxDistance = bottomLeftDistance;
    }

    if (bottomRightDistance > maxDistance) {
        maxDistance = bottomRightDistance;
    }

    if (maxDistance <= 0.0f) {
        *PixelAt<Pixel>(output, 0, 0) = *PixelAt<Pixel>(input, 0, 0);
        return;
    }

    const float globalMotionRatio = useGlobalMotion
        ? ComputeGlobalMotionRatio<Pixel>(
            input,
            previousInputs,
            previousInputCount,
            motionMultiplier,
            maxChannelValue)
        : 0.0f;

    for (A_long y = 0; y < height; ++y) {
        for (A_long x = 0; x < width; ++x) {
            const Pixel *currentPixel = PixelAt<Pixel>(input, x, y);
            float motionRatio = 0.0f;

            if (useGlobalMotion) {
                motionRatio = globalMotionRatio;
            } else {
                float motionTotal = 0.0f;
                A_long validMotionFrames = 0;

                for (A_long frameIndex = 0; frameIndex < previousInputCount; ++frameIndex) {
                    const PF_LayerDef *previousInput = previousInputs[frameIndex];

                    if (previousInput && previousInput->data && previousInput->width > 0 && previousInput->height > 0) {
                        const A_long previousX = ClampLong(x, 0, previousInput->width - 1);
                        const A_long previousY = ClampLong(y, 0, previousInput->height - 1);
                        const Pixel *previousPixel = PixelAt<Pixel>(*previousInput, previousX, previousY);

                        motionTotal += MotionAmount(*currentPixel, *previousPixel, maxChannelValue);
                        ++validMotionFrames;
                    }
                }

                if (validMotionFrames > 0) {
                    motionRatio = ClampFloat(
                        (motionTotal / static_cast<float>(validMotionFrames)) * motionMultiplier,
                        0.0f,
                        1.0f);
                }
            }

            const float dx = static_cast<float>(x) - clampedCenterX;
            const float dy = static_cast<float>(y) - clampedCenterY;
            const float distance = std::sqrt((dx * dx) + (dy * dy));
            const float rawDistanceRatio = distance / maxDistance;
            const float distanceRatio = rawDistanceRatio < 1.0f ? rawDistanceRatio : 1.0f;
            const float blurRatio = distanceRatio * distanceRatio * motionRatio;
            const A_long sampleCount = 1 + static_cast<A_long>((kMaxSampleCount - 1) * blurRatio);

            if (sampleCount <= 1 || distance <= 0.0f) {
                *PixelAt<Pixel>(output, x, y) = *currentPixel;
                continue;
            }

            const float dirX = dx / distance;
            const float dirY = dy / distance;
            const float blurDistance = blurStrength * blurRatio;

            float alpha = 0.0f;
            float red = 0.0f;
            float green = 0.0f;
            float blue = 0.0f;
            float weightTotal = 0.0f;

            for (A_long sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
                const float offsetRatio = SampleOffsetRatio(blurType, sampleIndex, sampleCount, x, y);
                const float sampleWeight = SampleWeight(blurType, offsetRatio);
                const A_long sampleX = ClampLong(
                    static_cast<A_long>(x + (dirX * blurDistance * offsetRatio)),
                    0,
                    width - 1);
                const A_long sampleY = ClampLong(
                    static_cast<A_long>(y + (dirY * blurDistance * offsetRatio)),
                    0,
                    height - 1);

                if (blurType == YAKUDODOUGA_BLUR_TYPE_DOWNSAMPLED) {
                    const A_long boxRadius = 1 + static_cast<A_long>(blurRatio * 3.0f);
                    AccumulateBoxSample<Pixel>(
                        input,
                        sampleX,
                        sampleY,
                        boxRadius,
                        sampleWeight,
                        alpha,
                        red,
                        green,
                        blue,
                        weightTotal);
                } else if (blurType == YAKUDODOUGA_BLUR_TYPE_MULTI_PASS) {
                    const float subStep = 1.0f / static_cast<float>(sampleCount);
                    const float subOffsets[3] = {
                        ClampFloat(offsetRatio - subStep, -1.0f, 1.0f),
                        offsetRatio,
                        ClampFloat(offsetRatio + subStep, -1.0f, 1.0f),
                    };
                    const float subWeights[3] = {0.25f, 0.5f, 0.25f};

                    for (A_long subIndex = 0; subIndex < 3; ++subIndex) {
                        const A_long subSampleX = ClampLong(
                            static_cast<A_long>(x + (dirX * blurDistance * subOffsets[subIndex])),
                            0,
                            width - 1);
                        const A_long subSampleY = ClampLong(
                            static_cast<A_long>(y + (dirY * blurDistance * subOffsets[subIndex])),
                            0,
                            height - 1);

                        AccumulatePointSample<Pixel>(
                            input,
                            subSampleX,
                            subSampleY,
                            sampleWeight * subWeights[subIndex],
                            alpha,
                            red,
                            green,
                            blue,
                            weightTotal);
                    }
                } else {
                    AccumulatePointSample<Pixel>(
                        input,
                        sampleX,
                        sampleY,
                        sampleWeight,
                        alpha,
                        red,
                        green,
                        blue,
                        weightTotal);
                }
            }

            if (weightTotal <= 0.0f) {
                *PixelAt<Pixel>(output, x, y) = *currentPixel;
                continue;
            }

            Pixel *destination = PixelAt<Pixel>(output, x, y);
            destination->alpha = static_cast<decltype(destination->alpha)>((alpha / weightTotal) + 0.5f);
            destination->red = static_cast<decltype(destination->red)>((red / weightTotal) + 0.5f);
            destination->green = static_cast<decltype(destination->green)>((green / weightTotal) + 0.5f);
            destination->blue = static_cast<decltype(destination->blue)>((blue / weightTotal) + 0.5f);
        }
    }
}

static void RenderMotionRadialBlur(
    const PF_LayerDef &input,
    const PF_LayerDef *const previousInputs[],
    A_long previousInputCount,
    PF_LayerDef &output,
    float centerX,
    float centerY,
    float blurStrength,
    float motionMultiplier,
    A_long blurType,
    PF_Boolean useGlobalMotion)
{
    if (PF_WORLD_IS_DEEP(&input)) {
        RenderMotionRadialBlurTyped<PF_Pixel16>(
            input,
            previousInputs,
            previousInputCount,
            output,
            centerX,
            centerY,
            blurStrength,
            motionMultiplier,
            blurType,
            useGlobalMotion,
            static_cast<float>(PF_MAX_CHAN16));
    } else {
        RenderMotionRadialBlurTyped<PF_Pixel8>(
            input,
            previousInputs,
            previousInputCount,
            output,
            centerX,
            centerY,
            blurStrength,
            motionMultiplier,
            blurType,
            useGlobalMotion,
            static_cast<float>(PF_MAX_CHAN8));
    }
}

} // namespace

static PF_Err About(
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output)
{
    (void)params;
    (void)output;

    AEGP_SuiteHandler suites(in_data->pica_basicP);

    suites.ANSICallbacksSuite1()->sprintf(
        out_data->return_msg,
        "%s v%d.%d\r%s",
        STR(StrID_Name),
        MAJOR_VERSION,
        MINOR_VERSION,
        STR(StrID_Description));

    return PF_Err_NONE;
}

static PF_Err GlobalSetup(
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output)
{
    (void)in_data;
    (void)params;
    (void)output;

    out_data->my_version = PF_VERSION(
        MAJOR_VERSION,
        MINOR_VERSION,
        BUG_VERSION,
        STAGE_VERSION,
        BUILD_VERSION);

    out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_WIDE_TIME_INPUT;
    out_data->out_flags2 = PF_OutFlag2_SUPPORTS_THREADED_RENDERING;

    return PF_Err_NONE;
}

static PF_Err ParamsSetup(
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output)
{
    (void)params;
    (void)output;

    PF_ParamDef def;
    AEFX_CLR_STRUCT(def);

    PF_ADD_POINT(
        STR(StrID_Center_Param_Name),
        YAKUDODOUGA_CENTER_X_DFLT,
        YAKUDODOUGA_CENTER_Y_DFLT,
        YAKUDODOUGA_RESTRICT_BOUNDS,
        CENTER_DISK_ID);

    AEFX_CLR_STRUCT(def);

    PF_ADD_FLOAT_SLIDERX(
        STR(StrID_Blur_Strength_Param_Name),
        YAKUDODOUGA_BLUR_STRENGTH_MIN,
        YAKUDODOUGA_BLUR_STRENGTH_MAX,
        YAKUDODOUGA_BLUR_STRENGTH_MIN,
        YAKUDODOUGA_BLUR_STRENGTH_MAX,
        YAKUDODOUGA_BLUR_STRENGTH_DFLT,
        PF_Precision_HUNDREDTHS,
        0,
        0,
        BLUR_STRENGTH_DISK_ID);

    PF_ADD_FLOAT_SLIDERX(
        STR(StrID_Motion_Multiplier_Param_Name),
        YAKUDODOUGA_MOTION_MULTIPLIER_MIN,
        YAKUDODOUGA_MOTION_MULTIPLIER_MAX,
        YAKUDODOUGA_MOTION_MULTIPLIER_MIN,
        YAKUDODOUGA_MOTION_MULTIPLIER_MAX,
        YAKUDODOUGA_MOTION_MULTIPLIER_DFLT,
        PF_Precision_HUNDREDTHS,
        0,
        0,
        MOTION_MULTIPLIER_DISK_ID);

    AEFX_CLR_STRUCT(def);

    PF_ADD_SLIDER(
        STR(StrID_Motion_Frame_Count_Param_Name),
        YAKUDODOUGA_MOTION_FRAME_COUNT_MIN,
        YAKUDODOUGA_MOTION_FRAME_COUNT_MAX,
        YAKUDODOUGA_MOTION_FRAME_COUNT_MIN,
        YAKUDODOUGA_MOTION_FRAME_COUNT_MAX,
        YAKUDODOUGA_MOTION_FRAME_COUNT_DFLT,
        MOTION_FRAME_COUNT_DISK_ID);

    AEFX_CLR_STRUCT(def);

    PF_ADD_POPUP(
        STR(StrID_Blur_Type_Param_Name),
        YAKUDODOUGA_BLUR_TYPE_CHOICES,
        YAKUDODOUGA_BLUR_TYPE_DFLT,
        STR(StrID_Blur_Type_Choices),
        BLUR_TYPE_DISK_ID);

    AEFX_CLR_STRUCT(def);

    PF_ADD_CHECKBOXX(
        STR(StrID_Global_Motion_Param_Name),
        false,
        0,
        GLOBAL_MOTION_DISK_ID);

    out_data->num_params = YAKUDODOUGA_NUM_PARAMS;

    return PF_Err_NONE;
}

static PF_Err Render(
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output)
{
    PF_Err err = PF_Err_NONE;
    PF_Err err2 = PF_Err_NONE;
    PF_ParamDef previousFrames[YAKUDODOUGA_MOTION_FRAME_COUNT_MAX];
    const PF_LayerDef *previousInputs[YAKUDODOUGA_MOTION_FRAME_COUNT_MAX];
    A_long checkedOutFrameCount = 0;
    (void)out_data;

    for (A_long frameIndex = 0; frameIndex < YAKUDODOUGA_MOTION_FRAME_COUNT_MAX; ++frameIndex) {
        AEFX_CLR_STRUCT(previousFrames[frameIndex]);
        previousInputs[frameIndex] = NULL;
    }

    const A_long requestedFrameCount = ClampLong(
        params[YAKUDODOUGA_MOTION_FRAME_COUNT]->u.sd.value,
        YAKUDODOUGA_MOTION_FRAME_COUNT_MIN,
        YAKUDODOUGA_MOTION_FRAME_COUNT_MAX);

    if (in_data->time_step > 0) {
        for (A_long frameOffset = 1; !err && frameOffset <= requestedFrameCount; ++frameOffset) {
            const A_long timeOffset = frameOffset * in_data->time_step;

            if (in_data->current_time >= timeOffset) {
                ERR(PF_CHECKOUT_PARAM(
                    in_data,
                    YAKUDODOUGA_INPUT,
                    in_data->current_time - timeOffset,
                    in_data->time_step,
                    in_data->time_scale,
                    &previousFrames[checkedOutFrameCount]));

                if (!err) {
                    previousInputs[checkedOutFrameCount] = &previousFrames[checkedOutFrameCount].u.ld;
                    ++checkedOutFrameCount;
                }
            }
        }
    }

    if (!err) {
        RenderMotionRadialBlur(
            params[YAKUDODOUGA_INPUT]->u.ld,
            previousInputs,
            checkedOutFrameCount,
            *output,
            static_cast<float>(FIX_2_FLOAT(params[YAKUDODOUGA_CENTER]->u.td.x_value)),
            static_cast<float>(FIX_2_FLOAT(params[YAKUDODOUGA_CENTER]->u.td.y_value)),
            static_cast<float>(params[YAKUDODOUGA_BLUR_STRENGTH]->u.fs_d.value),
            static_cast<float>(params[YAKUDODOUGA_MOTION_MULTIPLIER]->u.fs_d.value),
            params[YAKUDODOUGA_BLUR_TYPE]->u.pd.value,
            params[YAKUDODOUGA_GLOBAL_MOTION]->u.bd.value);
    }

    for (A_long frameIndex = 0; frameIndex < checkedOutFrameCount; ++frameIndex) {
        ERR2(PF_CHECKIN_PARAM(in_data, &previousFrames[frameIndex]));
    }

    return err;
}

extern "C" DllExport
PF_Err PluginDataEntryFunction2(
    PF_PluginDataPtr inPtr,
    PF_PluginDataCB2 inPluginDataCallBackPtr,
    SPBasicSuite *inSPBasicSuitePtr,
    const char *inHostName,
    const char *inHostVersion)
{
    (void)inSPBasicSuitePtr;
    (void)inHostName;
    (void)inHostVersion;

    PF_Err result = PF_Err_INVALID_CALLBACK;

    result = PF_REGISTER_EFFECT_EXT2(
        inPtr,
        inPluginDataCallBackPtr,
        "YakudoDouga",
        "YakudoDouga",
        "YakudoDouga",
        AE_RESERVED_INFO,
        "EffectMain",
        "https://www.adobe.com");

    return result;
}

PF_Err EffectMain(
    PF_Cmd cmd,
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output,
    void *extra)
{
    PF_Err err = PF_Err_NONE;
    (void)extra;

    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                err = About(in_data, out_data, params, output);
                break;

            case PF_Cmd_GLOBAL_SETUP:
                err = GlobalSetup(in_data, out_data, params, output);
                break;

            case PF_Cmd_PARAMS_SETUP:
                err = ParamsSetup(in_data, out_data, params, output);
                break;

            case PF_Cmd_RENDER:
                err = Render(in_data, out_data, params, output);
                break;

            default:
                break;
        }
    } catch (PF_Err &thrown_err) {
        err = thrown_err;
    }

    return err;
}
