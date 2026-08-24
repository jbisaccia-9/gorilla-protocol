# Pinned Toolchain

- Unreal Engine: 5.8, exact installed build/changelist to be recorded after installation
- Project engine association: `5.8`
- macOS recommended Xcode: 26.1.1
- macOS minimum Xcode: 16.4
- C++ build settings: engine `Latest` for the pinned UE 5.8 installation
- Rendering baseline: deferred renderer, SM6, Lumen, VSM, TSR

Do not upgrade the Engine installation, BuildSettings, include order, compiler,
or platform SDK in CI without a reviewed toolchain change and a full clean build.
