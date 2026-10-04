# Gorilla Protocol: Ubuntu Setup README

Last checked: 2026-10-04. For the person and coding agent working on the Linux PC.
Use this for installation; use [the handoff](Docs/LINUX_HANDOFF.md) for the game
direction, Git checkpoint, agent assignments, and usage limits.

**Unreal Engine runs natively on Linux. Epic Games Launcher does not currently
have a supported Ubuntu version.** Downloading a Windows `.exe` or Mac `.dmg`
on Ubuntu will not install a native launcher. Epic lists Windows and macOS in
its [Launcher requirements](https://www.epicgames.com/help/c-32735058/c-36403860/a22050085?lang=en-US).

## 1. What Goes Where

| Item | Linux PC | Mac | Download / purpose |
| --- | --- | --- | --- |
| Git + Git LFS | Required | Only for working with this repo | Ubuntu packages; downloads source and large Blender assets. |
| Blender | Native Linux archive | Existing install can stay | [Official 5.2 release archive](https://download.blender.org/release/Blender5.2/); prefer 5.2.1 to match saved art. |
| Unreal Engine | Native precompiled Linux ZIP | Compatible Mac engine for Lyra acquisition if needed | [Epic Linux downloads](https://www.unrealengine.com/en-US/linux); select the agreed engine version. |
| Epic Games Launcher | Do not install for this workflow | Required for the sample acquisition route | [Official Launcher download](https://store.epicgames.com/download). |
| Lyra Starter Game | Complete project, compiled locally | Download/create the official sample here first | [Epic's Lyra instructions](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine). |
| Unreal native toolchain | Engine-provided setup | Not a substitute for Linux build tools | Used by Unreal's build scripts, not a random system Clang. |
| VS Code / Rider | Optional | Optional | An IDE is not required to run the supplied build script. |

No Java runtime, Windows cross-compilation SDK, Wine, Heroic, paid GPU VM, or cloud
API key is needed for this native workflow. Do not install these to resolve a
missing Launcher. Blender modeling can proceed while Lyra acquisition is pending.
Do not download a third-party repack of Unreal or Lyra.

**Version decision:** the repository targets UE 5.8; earlier Linux builds used
5.8.1. Reuse a working 5.8.1 install if available. Download a Lyra release for that
engine family, not simply the latest sample. If Epic only offers a different
patch/version, record the available pair and agree on a migration before installing
or converting projects. Do not switch engine families silently. The saved Blender
art was inspected with 5.2.1; verify a different version before resaving the source.

## 2. Check Before Downloading

Run in an Ubuntu desktop Terminal, not the Mac terminal. Commands below assume
Bash: enter `bash` once if using a different shell. Run one block at a time and
stop on an error. `sudo` requests your Linux password locally; never send it to an
agent. Do not use `sudo` to run Blender, Unreal, Git, or project builds.

```bash
cat /etc/os-release
uname -m
free -h
df -h "$HOME"
lspci -nnk | grep -A3 -Ei 'VGA|3D|Display'
find "$HOME/Unreal" "$HOME/Downloads" /opt -maxdepth 7 \
  -type f -path '*/Engine/Binaries/Linux/UnrealEditor' -print 2>/dev/null
```

Missing search directories are harmless; no matches means discovery found no
engine there, not proof that the machine has no engine anywhere. Inspect any
existing project checkout before downloading another copy.

These instructions target an Intel/AMD `x86_64` PC. If `uname -m` says `aarch64`
or anything else, stop: do not download the x64 files below.

Epic recommends 32 GB RAM and 8 GB or more VRAM for Linux development; its current
requirements list NVIDIA 570+ or AMD RADV 24.2.8+ (25.0.0+ recommended). Check the
[version-specific requirements](https://dev.epicgames.com/documentation/en-us/unreal-engine/linux-development-requirements-for-unreal-engine)
against the actual hardware. A previous 16 GB machine may compile with reduced
parallelism, but that is not a guarantee of acceptable game/editor performance.
Reserve space for archives, extraction, Lyra, build output, and shader caches;
200 GB free is a planning allowance, not an official minimum or guaranteed total.

For NVIDIA, use Ubuntu's **Software & Updates > Additional Drivers**, choose a
compatible tested desktop driver, apply, and reboot when prompted. Do not mix a
downloaded `.run` installer with Ubuntu-managed drivers. AMD/Intel use their
appropriate Ubuntu graphics stack, not the NVIDIA packages. Follow
[Ubuntu's driver guide](https://ubuntu.com/desktop/docs/en/latest/how-to/graphics/install-nvidia-drivers/)
if a change is necessary; do not replace a working driver without evidence.

Install basic utilities from Ubuntu's configured repositories:

```bash
sudo apt update
sudo apt install git git-lfs curl ca-certificates unzip xz-utils \
  python3 ripgrep file pciutils vulkan-tools
```

Review the package changes before approving. If a package is unavailable, inspect
the Ubuntu release/repositories rather than adding arbitrary PPAs. Then check:

```bash
git --version
git lfs version
python3 --version
vulkaninfo --summary
```

Vulkan should enumerate the intended physical GPU, not only a software renderer
such as llvmpipe/lavapipe. Run in the logged-in desktop session; a headless SSH
session is not equivalent. On NVIDIA, `nvidia-smi` is an additional driver check.
Resolve driver/VRAM problems before blaming the game or starting long builds.

## 3. Get The Right Repository Branch

For a new checkout only, run each line after the previous succeeds:

```bash
mkdir -p "$HOME/Projects"
cd "$HOME/Projects"
git lfs install
git clone --branch codex/blender-coastal-slice \
  https://github.com/jbisaccia-9/gorilla-protocol.git gorilla-protocol-coastal
cd gorilla-protocol-coastal
git lfs pull
git lfs fsck
git status --short --branch
```

If the folder already exists, open it and inspect `git status --short --branch`
and `git remote -v`. Preserve changes. Only on a clean checkout of the correct
remote: `git fetch origin`, switch to `codex/blender-coastal-slice`, then
`git pull --ff-only` and `git lfs pull`. If the branch is missing locally, use
`git switch --track origin/codex/blender-coastal-slice`. Stop if branches diverge;
do not force-reset. A GitHub "Download ZIP" is not the preferred recovery path.

In that repository's terminal, set the path for the later commands:

```bash
export GP_ROOT="$PWD"
test -f "$GP_ROOT/Docs/LINUX_HANDOFF.md"
file "$GP_ROOT/Art/CoastalSlice/Gorilla_Coastal_Encounter.blend"
```

The last file must be real Blender data, not a tiny ASCII Git LFS pointer. If LFS
reports an authentication/quota problem, fix that first instead of rebuilding all
art. Keep machine-specific settings outside the repository. Shell variables here
last only for this terminal session; re-establish them after opening a new one.

## 4. Blender On Ubuntu

Prefer the official portable Linux x64 archive, avoiding accidental use of an
older Ubuntu package or a sandboxed package with different filesystem access.
The [official release listing](https://download.blender.org/release/Blender5.2/)
lists `blender-5.2.1-linux-x64.tar.xz`. Download that file in your browser. Do not
choose the macOS ARM/Intel or Windows build. Verify against the publisher's
checksum file from the same release directory when available; stop on a mismatch.

For a first-time extraction (do not overwrite an existing installation):

```bash
mkdir -p "$HOME/Applications"
test ! -e "$HOME/Applications/blender-5.2.1-linux-x64" && \
  tar -xJf "$HOME/Downloads/blender-5.2.1-linux-x64.tar.xz" \
    -C "$HOME/Applications"
export BLENDER_BIN="$HOME/Applications/blender-5.2.1-linux-x64/blender"
"$BLENDER_BIN" --version
```

If the browser renamed the archive, choose its actual filename, not a wildcard
that might select several downloads. If the installation already exists, skip
extraction and verify that binary. Keep the entire extracted Blender folder.

Open the source geometry, then close Blender normally before inspecting in batch:

```bash
"$BLENDER_BIN" "$GP_ROOT/Art/CoastalSlice/Gorilla_Coastal_Encounter.blend"
```

```bash
"$BLENDER_BIN" -b "$GP_ROOT/Art/CoastalSlice/Gorilla_Coastal_Encounter.blend" \
  --python-exit-code 1 --python "$GP_ROOT/Scripts/Blender/inspect_scene.py"
```

Expected: `SCENE_CHECKS` reports both guards, gorilla arms, banana charm, and art
revision 2. This updates the JSON reports; review the diff. It does not test game
logic. Only add `-- --render` when a small new render is needed. No full scene
rebuild is needed to open or inspect the saved asset.

The root `.command` files are **Mac launchers**, not Linux installers. Also do not
run the obsolete Java/web/graybox instructions to launch this new art direction.

## 5. Native Unreal On Ubuntu

If an existing editor works, inspect its `Engine/Build/Build.version` and reuse it.
Otherwise sign in to [Epic's Linux downloads](https://www.unrealengine.com/en-US/linux)
in your browser. Download the **precompiled Linux engine ZIP** for the agreed
version, not the Launcher installer, source ZIP, or Windows cross-compile SDK.
Epic's [Linux quickstart](https://dev.epicgames.com/documentation/en-us/unreal-engine/linux-development-quickstart-for-unreal-engine)
documents this Launcher-free installation route.

1. In Ubuntu Files, extract the ZIP to a new folder under `Home/Unreal` on the
   internal Linux disk. Wait for extraction to finish; do not open files inside
   the archive viewer. Avoid building on a USB FAT/exFAT drive or network share.
2. Open the extracted folder. Find the directory that directly contains `Engine`.
   That directory is `UE_ROOT`, even if there is an extra enclosing folder.
3. In the same Bash terminal, enter the actual absolute path when prompted:

```bash
read -r -p 'Folder directly containing Engine: ' UE_ROOT
export UE_ROOT
test -f "$UE_ROOT/Engine/Build/Build.version"
python3 -m json.tool "$UE_ROOT/Engine/Build/Build.version"
test -x "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"
test -x "$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh"
```

Type a full path such as your own home directory path, without surrounding quotes
at this prompt. Do not type a literal `~` or `$HOME` into `read`: it does not expand
them. If a `test` fails, stop and fix the path/extraction/permissions first.

Launch the editor without an old Gorilla project attached:

```bash
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"
```

Confirm the project browser opens, then close it. Do not create a random blank
project to substitute for Lyra. For C++ compilation, use this engine's native
toolchain. UE 5.8's requirements identify v26 / Clang 20.1.8; do not replace it
with whatever `clang` happens to be on PATH. The requirements page is more specific
than the quickstart's older compiler example.

If the bundled toolchain is missing, verify and run its setup script:

```bash
test -f "$UE_ROOT/Engine/Build/BatchFiles/Linux/SetupToolchain.sh"
bash "$UE_ROOT/Engine/Build/BatchFiles/Linux/SetupToolchain.sh"
```

If that script is absent, stop and identify the distribution/version; do not
invent the path or fall back to compiling the whole engine. Installed builds and
source builds have different setup needs. Let the engine's build scripts select
their bundled .NET SDK; installing Mono or arbitrary Clang/.NET packages is not
the default fix. No full Unreal source build is planned for this setup.

## 6. Acquire Lyra On The Mac

This is a one-time sample download step, not moving development back to the Mac.
Use your own Epic account and accept any licenses yourself.

1. On the Mac, install/open the [official Epic Games Launcher](https://store.epicgames.com/download).
2. In its Unreal Engine Library, install a compatible engine for the Lyra version
   selected in section 1 if none is present. Check the download/storage estimate
   first. An engine download on the Mac may be required even though Linux will
   compile the game; do not promise a Launcher-only download will suffice.
3. Follow [Epic's Lyra page](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine)
   to the official **Lyra Starter Game** Fab listing. Add it to your library.
4. In Launcher, open **Unreal Engine > Library > Fab Library**, locate Lyra, and
   choose **Create Project** for the matching version. Name it `LyraStarterGame`.
5. Wait for completion. Locate the folder containing `LyraStarterGame.uproject`,
   `Content`, `Config`, `Source`, and `Plugins`. Do not copy only the `.uproject`
   or `Content`: the shooter assets also live under plugins.

If Lyra is absent from the library, check the account, claimed listing, compatible
installed engine, and library refresh. A Fab browser listing or an in-editor Fab
window alone is not proof that the complete sample was downloaded. If the Mac
cannot acquire the compatible version, record that blocker instead of repeatedly
trying Wine, unofficial launchers, or public sample mirrors.

## 7. Transfer Lyra Without A Flash Drive

Keep Epic's project **outside the public Gorilla repo** on both devices. Use a
private cloud-drive upload/download or an authenticated local file transfer. No
public share link, GitHub upload, or open HTTP server is needed.

On the Mac, open Terminal, enter `bash`, and run this once. Supply the actual
project folder path at the prompt (no surrounding quotes):

```bash
read -r -p 'Full path to the downloaded LyraStarterGame folder: ' LYRA_SOURCE
test -f "$LYRA_SOURCE/LyraStarterGame.uproject"
```

Only after that test succeeds:

```bash
test ! -e "$HOME/Downloads/LyraStarterGame-transfer.tar.gz" && \
  COPYFILE_DISABLE=1 tar -czf "$HOME/Downloads/LyraStarterGame-transfer.tar.gz" \
    --exclude='Binaries' --exclude='Intermediate' --exclude='Saved' \
    --exclude='DerivedDataCache' --exclude='.DS_Store' \
    -C "$LYRA_SOURCE" .
shasum -a 256 "$HOME/Downloads/LyraStarterGame-transfer.tar.gz"
```

This keeps source/content/plugin folders but omits generated Mac outputs, including
nested plugin caches. Use it for the untouched official Lyra sample, not an
arbitrary project with binary-only third-party plugins. If the transfer archive
already exists, inspect it; do not assume it is the new download or overwrite it.

Transfer the archive privately. On Linux, with it in `Downloads`, verify the hash
matches the Mac's output, then inspect it before extraction:

```bash
sha256sum "$HOME/Downloads/LyraStarterGame-transfer.tar.gz"
tar -tzf "$HOME/Downloads/LyraStarterGame-transfer.tar.gz" | head -40
```

Expected entries include `./LyraStarterGame.uproject` and the project directories.
Extract only into a new destination, never over an existing modified project:

```bash
mkdir -p "$HOME/Projects"
mkdir "$HOME/Projects/LyraStarterGame" && \
  tar -xzf "$HOME/Downloads/LyraStarterGame-transfer.tar.gz" \
    -C "$HOME/Projects/LyraStarterGame"
export LYRA_ROOT="$HOME/Projects/LyraStarterGame"
test -f "$LYRA_ROOT/LyraStarterGame.uproject"
```

If that directory already exists, stop and inspect it. Do not delete it to satisfy
the command. Mac engine binaries cannot run on Linux; transfer the sample project,
not the Mac Unreal Engine installation. The Linux build creates its own outputs.

## 8. Validate Before Building

In the Linux terminal with `GP_ROOT`, `UE_ROOT`, and `LYRA_ROOT` set:

```bash
cd "$GP_ROOT"
./Scripts/Lyra/validate_lyra_baseline.sh
```

Expected: `Lyra baseline validation passed`. This verifies the project and key
ShooterCore/ShooterMaps files, not the engine version or successful gameplay.
The resolver can fall back to another installation if an override is invalid;
confirm the printed Lyra path is the intended one. Recheck all three paths before
launching if multiple installs exist. Then run the build **once**:

```bash
./Scripts/Lyra/build_and_play_baseline_linux.sh
```

Expected: it builds `LyraEditor`, then launches the official `L_Expanse` shooter
with timing counters. This is **Lyra, not the completed gorilla game**. The initial
window is 1600x900 and runs editor `-game`; it is not a packaged Shipping benchmark.
Use [Playability Gate](Docs/PLAYABILITY_GATE.md) for the later packaged 1080p tests.
Do not start the old Gorilla bootstrap just because Lyra is not installed yet.

## 9. Common Linux Failures

| Symptom | Next action; do not restart the entire setup |
| --- | --- |
| Launcher downloads `.exe` / `.dmg` | Wrong platform for the Launcher; use sections 5 and 6. |
| `No such file or directory` | Inspect the exact failing path and enclosing directory. Use quoted variables; Linux names are case-sensitive. Never type placeholder paths literally. |
| `Permission denied` | Inspect `ls -l` and mount options with `findmnt -T` for that file. Use an executable Linux filesystem; do not use recursive `chmod 777` or run the app as root. |
| Unreal binary exists but cannot execute | Check `file` output, CPU architecture, and loader/library error. A Mac/Windows executable is not a Linux binary. |
| Blender cannot open `.blend` | Check Git LFS first and Blender version second. Do not regenerate missing downloads. |
| Shader compiling / apparent long freeze | Check CPU/disk activity and the current log before deciding it is hung. Preserve caches. Initial compilation time is hardware-dependent, not a promised ETA. |
| Black/empty Expanse editor viewport | Lyra uses World Partition. In the editor, load cells or start its experience; an empty viewport alone is not a missing map. See Epic's Lyra page. |
| Missing ShooterCore / ShooterMaps | Incomplete sample or wrong root. Transfer the full official project, including Plugins; no need to redownload the Linux engine. |
| Compile fails or process is killed | Capture the first actual error and available RAM/disk. Inspect for out-of-memory before adding parallel builds or agents. |
| Vulkan / GPU error or extreme lag | Confirm the physical GPU, driver, VRAM, and scalability; test native local rendering before cloud streaming. |

For a failed Lyra run, keep diagnostic output local:

```bash
tail -n 100 "$LYRA_ROOT/Saved/Logs/Lyra.log"
tail -n 100 "$HOME/.config/Epic/UnrealBuildTool/Log.txt"
```

A log may not exist if execution failed earlier; inspect the actual `Saved/Logs`
folder. Share the first relevant error and a short surrounding excerpt, not an
entire log containing personal paths. Do not commit logs, account details, or
downloaded Epic content to the public repository.

## 10. Agent Completion Checklist

- [ ] Correct Git branch and real LFS assets, with existing changes preserved.
- [ ] Actual OS/GPU/RAM/disk and tool versions recorded privately.
- [ ] Blender opens the scene and structural inspection succeeds.
- [ ] Native Unreal opens; engine/Lyra versions and toolchain are verified.
- [ ] Complete Lyra project validates and the untouched experience launches.
- [ ] Failures or missing account actions are recorded without claiming success.
- [ ] Handoff's bounded two-agent workflow starts only after tasks are ready.

Use small checks and incremental builds, reuse caches, and do not launch subagents
to duplicate installation work. After two failed attempts at the same problem,
diagnose it rather than repeating downloads. No purchases, paid hosting, automatic
credit resets, or OS upgrades are authorized by this guide.

These instructions were checked against official documentation and this repo's
scripts. Shell syntax can be checked on the Mac; actual Ubuntu installation,
account downloads, graphics support, and Lyra execution must be verified on the
Linux machine. They have not been run there as part of writing this README.
