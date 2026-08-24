# Third-Party Notices

No third-party runtime assets or source distributions are currently vendored.

The optional browser-streaming deployment downloads or runs these components:

- Epic Games Pixel Streaming Infrastructure, MIT License
- Caddy, Apache License 2.0
- CoTURN, BSD 3-Clause License

Their source and container images remain external dependencies. Preserve their
license notices in every deployed image and review the exact pinned versions
before production use.

Every imported model, texture, animation, sound, voice recording, plugin, or font
must be recorded in `AssetManifest.csv` before it can be committed or packaged.
