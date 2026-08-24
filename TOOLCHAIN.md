# Pinned Toolchain

- Unreal Engine: 5.8, exact installed build/changelist to be recorded after installation
- Project engine association: `5.8`
- macOS recommended Xcode: 26.1.1
- macOS minimum Xcode: 16.4
- C++ build settings: engine `Latest` for the pinned UE 5.8 installation
- Rendering baseline: deferred renderer, SM6, Lumen, VSM, TSR
- Browser delivery: Pixel Streaming 2 with Pixel Streaming Infrastructure UE5.8
- Reviewed infrastructure commit: `d063f92e69750bc2eafd7e88011444cfddef1cbf`
- Signaling setup minimum Node.js: 22.14.0; Epic's Dockerfile currently resolves
  its `node:lts` base separately and must be re-audited when the image is rebuilt
- TURN container baseline: `coturn/coturn:4.17.2-r0`, manifest digest
  `sha256:aa68aab64a3b929d57fc2924c98ea447bf996cf8dade2508e7b71eaf23f1f14e`
- HTTPS proxy baseline: `caddy:2.10.2-alpine`, manifest digest
  `sha256:4c6e91c6ed0e2fa03efd5b44747b625fec79bc9cd06ac5235a779726618e530d`

Do not upgrade the Engine installation, BuildSettings, include order, compiler,
or platform SDK in CI without a reviewed toolchain change and a full clean build.
