# YakudoDouga

YakudoDouga is an Adobe After Effects effect plugin. The plugin applies a motion-reactive radial blur whose strength increases toward the edge of the frame.

## Repository Placement

This project is intended to be placed under the Adobe After Effects SDK `Examples` directory on both macOS and Windows.

Expected layout:

```text
AfterEffectsSDK/
  Examples/
    YakudoDouga/
      Mac/
      Win/
      Source/
```

The project files use the same relative layout as the AE SDK examples, so building from under `Examples/YakudoDouga` avoids broken SDK header, library, and resource paths.

## Download The After Effects SDK

Download the Adobe After Effects SDK from the official Adobe Developer site:

```text
https://developer.adobe.com/after-effects/
```

Open the page, choose the SDK download link, then download and extract the SDK for your After Effects version.

After extracting the SDK, locate its `Examples` directory. The repository should be cloned directly under that directory:

```text
AfterEffectsSDK/
  Examples/
    YakudoDouga/
```

The exact extracted SDK folder name may include the SDK version and platform, for example:

```text
ae25.6_61.64bit.AfterEffectsSDK/
  Examples/
```

## Clone

From the AE SDK `Examples` directory:

```sh
git clone https://github.com/Nanamiiiii/YakudoDouga-AE.git YakudoDouga
```

After cloning, confirm the project exists at:

```text
AfterEffectsSDK/Examples/YakudoDouga
```

## Build On macOS

Requirements:

- macOS
- Xcode
- Adobe After Effects SDK
- Adobe After Effects installed for local plugin testing

Steps:

1. Clone the repository into the SDK `Examples` directory.
2. Open the Xcode project under `YakudoDouga/Mac`.
3. Select the `YakudoDouga` scheme.
4. Build the project with Xcode.

The macOS build product is a plugin bundle:

```text
YakudoDouga.plugin
```

For testing in After Effects, copy or symlink the built `.plugin` bundle into the After Effects `Plug-ins` directory, then restart After Effects.

Example destination:

```text
/Applications/Adobe After Effects 2025/Plug-ins/YakudoDouga.plugin
```

## Build On Windows

Requirements:

- Windows
- Visual Studio
- Adobe After Effects SDK
- Adobe After Effects installed for local plugin testing

Install Visual Studio with the following workload and components:

- Workload: `Desktop development with C++`
- MSVC C++ x64/x86 build tools
- Windows SDK
- C++ CMake tools for Windows, if your Visual Studio installation does not already include the required native build support

Steps:

1. Clone the repository into the SDK `Examples` directory.
2. Open the Visual Studio solution or project under `YakudoDouga/Win`.
3. Select an x64 configuration.
4. Build the project in Visual Studio.

The Windows build product is:

```text
YakudoDouga.aex
```

For testing in After Effects, copy the built `.aex` file into the After Effects `Plug-ins` directory, then restart After Effects.

Example destination:

```text
C:\Program Files\Adobe\Adobe After Effects 2025\Support Files\Plug-ins\YakudoDouga.aex
```

## Notes

- macOS After Effects plugins are `.plugin` bundles.
- Windows After Effects plugins are `.aex` files.
- Keep the project under the AE SDK `Examples` directory unless the project paths are updated.
- If the plugin parameter layout or PiPL metadata changes, keep the plugin build version and PiPL effect version in sync.
