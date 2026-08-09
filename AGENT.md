# YakudoDouga Plugin Context

## Project

YakudoDouga is an Adobe After Effects effect plugin based on the AE SDK sample structure. The main implementation lives in:

- `YakudoDouga/YakudoDouga.cpp`
- `YakudoDouga/YakudoDouga.h`
- `YakudoDouga/YakudoDouga_Strings.cpp`
- `YakudoDouga/YakudoDouga_Strings.h`
- `YakudoDouga/YakudoDougaPiPL.r`

Do not edit `.xcodeproj/project.pbxproj` directly while Xcode is open. Use Xcode for project-level setting changes.

## Current Effect Behavior

The effect is a motion-reactive radial blur:

- Blur direction is radial from a user-selected `Center`.
- Blur strength increases with distance from `Center`.
- Motion is computed from RGB differences between the current frame and previous checked-out frames.
- Motion affects blur strength only. It does not determine blur direction.
- `Global Motion` off uses per-pixel motion differences.
- `Global Motion` on uses the whole-frame average motion amount as a uniform motion multiplier across the frame.

The center point must be treated as AE layer coordinates, not percentages. `PF_Param_POINT` values are read with `FIX_2_FLOAT(params[YAKUDODOUGA_CENTER]->u.td.x_value)` and `y_value`, then clamped to the frame bounds before distance calculations.

## Parameters

Defined in `YakudoDouga/YakudoDouga.h` and registered in `ParamsSetup`:

- `Center`: point control for radial blur origin.
- `Blur Strength`: base blur distance, default `80.0`, range `0.0` to `240.0`.
- `Motion Multiplier`: scales detected motion, default `1.0`, range `0.0` to `5.0`.
- `Motion Frames`: number of previous frames used for motion detection, default `1`, range `1` to `8`.
- `Blur Type`: popup with six choices.
- `Global Motion`: checkbox for whole-frame motion strength.

Blur type choices:

1. `Linear Average`
2. `Gaussian`
3. `Outward`
4. `Jittered Gaussian`
5. `Multi-pass`
6. `Downsampled`

## Rendering Notes

The main render path:

1. `Render` checks out up to `Motion Frames` previous input frames using `PF_CHECKOUT_PARAM`.
2. It calls `RenderMotionRadialBlur`.
3. `RenderMotionRadialBlur` dispatches to `RenderMotionRadialBlurTyped<PF_Pixel16>` for deep color or `RenderMotionRadialBlurTyped<PF_Pixel8>` otherwise.
4. Previous frames are checked in with `PF_CHECKIN_PARAM`.

The effect declares:

- `PF_OutFlag_DEEP_COLOR_AWARE`
- `PF_OutFlag_WIDE_TIME_INPUT`
- `PF_OutFlag2_SUPPORTS_THREADED_RENDERING`

Because the effect reads previous frames, keep `PF_OutFlag_WIDE_TIME_INPUT`.

## Versioning

Keep `BUILD_VERSION` in `YakudoDouga/YakudoDouga.h` and `AE_Effect_Version` in `YakudoDouga/YakudoDougaPiPL.r` in sync when changing the plugin's parameter surface or PiPL-relevant metadata.

Current state:

- `BUILD_VERSION 9`
- `AE_Effect_Version 524297`

## Known Design Decisions

The current `Center` default is `50, 50` in layer coordinates. If a true frame-center default is needed, do not convert the point parameter back to percentages. Prefer either:

- Treat the untouched default `50,50` as a special "use frame center" case during render, accepting that an explicit `50,50` cannot be distinguished.
- Add proper initialization/state handling so the parameter is initialized to the frame center on first application.

The first option is simpler; the second is stricter but requires more AE plugin state handling.

## Build And Validation

Use Xcode's active scheme to build. In this environment, prefer the `BuildProject` Xcode tool. After changing code, also check build warnings.

The last confirmed build after the center-coordinate fix succeeded with no warnings.
