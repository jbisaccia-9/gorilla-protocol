# Linux Handoff: Start Here

Updated 2026-10-04. This document preserves the project decisions and next actions
from the Mac conversation. It is not a claim that the full chat history transfers
between devices. Read this before building or delegating work.

For exact downloads, platform differences, prerequisites, private Lyra transfer,
and launch commands, follow [the Linux setup README](../README_LINUX_SETUP.md).
There is no supported native Ubuntu Epic Games Launcher in this workflow: acquire
the native Linux engine directly, and acquire the Lyra sample through the Mac
Launcher before transferring the complete project privately. Do not spend the
setup budget trying to install a Windows/Mac launcher on Ubuntu.

## Verified Starting Point

- Repository: https://github.com/jbisaccia-9/gorilla-protocol
- Continue branch: `codex/blender-coastal-slice`, not the older `main` prototype.
- Last implementation commit before this handoff: `6688f08`, "Refine coastal
  foliage fur and storm lighting in Blender". The handoff commit is a later
  documentation-only change; do not reset to `6688f08`.
- The implementation commit was verified on GitHub. The Mac worktree was clean
  before this documentation update. Recheck the Linux worktree before edits.
- Previous source-art commit: `c613038`; Lyra reset decision: `b907546`.
- Rejected old gameplay is retained at `pre-lyra-prototype-2026-08-28` and in Git
  history. Do not revive it as the new gameplay foundation.
- The flash drive is not needed to recover committed source and LFS assets.
  Uncommitted files, local engine installs, credentials, and downloaded Epic
  content are not recovered by cloning this repository.

**Current state: real Blender source art; not a finished or verified playable
game.** No new skinned guard rig, combat animation package, material bake/export
pipeline, or Lyra integration was completed after `6688f08`. Discussion of those
steps was a plan, not an implementation.

## What The User Expects

An original first-person tactical-comedy shooter: Bruno is a gorilla who speaks
only Italian. The feel draws on classic spy shooters, but visuals, animation,
controls, and enemy behavior must be contemporary. It should be fun and silly
through character and physical comedy, not look like a primitive cartoon demo.
Do not copy franchise characters, weapons, branding, or other protected assets.

The binding visual reference is [Approved Visual Target](VISUAL_TARGET.md) and
its [image](Art/vertical-slice-visual-target.png). It is concept art, not a game
screenshot. Match its visual language, not a promise of automatic console quality:

- Stormy Mediterranean cliffside intelligence facility, ocean, cypress, stone,
  concrete, steel, communications equipment, and readable tactical routes.
- Cool blue exterior, warm amber interiors, restrained red alarm lamps, wet PBR
  surfaces with distinct roughness rather than everything looking like plastic.
- Credible furry gorilla forearms, hands that grip the weapon correctly, a
  compact suppressed carbine, and one small banana charm.
- Dark-uniformed guards that navigate, aim, attack, react, take damage, and die.
- Responsive first-person movement, aiming, fire, reload, damage feedback,
  Italian dialogue/subtitles, an objective, and a repeatable win/lose/restart loop.

The user repeatedly rejected static or crude grayboxes, laggy streaming, empty
editor maps, broken rebuild instructions, and images presented as game progress.
Do not ask them to spend hours repeating an untested setup sequence. Diagnose
the first actionable error, preserve logs, and validate fixes before proceeding.

## Recover On Linux

Use an existing checkout if present: inspect `git status --short --branch` and
`git remote -v` first. Preserve local changes. Do not reset, clean, overwrite,
or reclone over an existing directory to make an error disappear.

For a fresh checkout, after Git and Git LFS are installed:

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

Run each command only after the previous one succeeds. If the destination exists,
inspect that checkout instead. If Git LFS is absent, install it using this Linux
distribution's package manager; do not guess the distribution or use `sudo`
without the necessary approval. A small LFS pointer file is not a usable `.blend`.
Use normal GitHub authentication if access is requested; never paste tokens here.

Open this directory in the Linux coding session and ask:

> Read Docs/LINUX_HANDOFF.md and inspect the current Git state. Continue Gorilla
> Protocol from the Blender coastal branch. Follow the bounded two-agent plan
> and efficiency rules. Verify this machine's tools before builds. Deliver one
> tested coastal encounter, not another prototype or static render marketed as
> gameplay. Preserve existing work and report blockers honestly.

## First Linux Session

1. Read this handoff, [Art README](../Art/README.md), [Visual Target](VISUAL_TARGET.md),
   and [Playability Gate](PLAYABILITY_GATE.md). Then read only code needed for the
   chosen task. Older broad roadmaps are background, not authorization to build
   every planned feature or restore the yacht setting.
2. Inventory the actual OS, GPU/driver/VRAM, RAM, free disk, Blender version,
   Unreal install, and Lyra installation. Keep machine-specific paths and logs
   local. The user now has Linux available; its current installs are unverified.
3. Confirm LFS assets exist, load the saved Blender scene, and run its structural
   inspection. Do not regenerate all geometry just to open an existing scene.
4. Confirm a compatible Unreal/Lyra pair. UE 5.8.1 was used in earlier Linux
   attempts; do not assume it or the matching official Lyra sample is installed.
   Lyra was not downloaded when work paused. Obtain it through the user's Epic
   account; do not substitute an unofficial copy or commit Epic sample content.
5. Run and measure untouched Lyra before customization, as Gate A requires.
   If Epic access or engine installation is blocked, continue independent asset
   work and record the blocker; do not claim the baseline passed.
6. Select one bounded work package, assign file ownership, and start the two-agent
   workflow only after dependencies and acceptance checks are clear.

## Source Assets And Commands

| Path | Purpose and actual state |
| --- | --- |
| `Art/CoastalSlice/Gorilla_Coastal_Encounter.blend` | Saved editable scene, art revision 2; not an Unreal map. |
| `Art/CoastalSlice/encounter-refined.png` | Latest Blender render; `encounter-preview.png` is the earlier pass. |
| `Art/CoastalSlice/scene-checks.json` | Recorded asset checks; gameplay and Unreal flags are explicitly false. |
| `Scripts/Blender/build_coastal_slice.py` | Reproducible environment/characters builder; includes refinement. |
| `Scripts/Blender/refine_coastal_slice.py` | Refines existing foliage, fur, and lighting without a full rebuild. |
| `Scripts/Blender/inspect_scene.py` | Asset/triangle checks, optional small preview render. |
| `Scripts/Blender/bruno.py` | First-person gorilla hands/arms and weapon geometry; not a complete animated character rig. |
| `Scripts/Blender/guards.py` | Original detailed guards with rigid transform animation, not skinned locomotion/combat rigs. |
| `Scripts/Lyra/build_and_play_baseline_linux.sh` | Builds official external Lyra and launches Expanse; does not launch a finished Gorilla game. |

The scene was authored and inspected with Blender 5.2.1 LTS on the Mac. Recheck
compatibility on Linux before modifying or resaving it. Blender units are meters;
the scene uses Z up. Viewmodel geometry is camera-local and guards face local -Y:
inspect the source before transforming/exporting either asset.

Run from the repository root with Blender on PATH (or its verified full path):

```bash
# Structural inspection only; updates tracked JSON reports, not the .blend.
blender -b Art/CoastalSlice/Gorilla_Coastal_Encounter.blend \
  --python-exit-code 1 --python Scripts/Blender/inspect_scene.py

# Open the actual source scene interactively.
blender Art/CoastalSlice/Gorilla_Coastal_Encounter.blend
```

The root `.command` launchers use macOS paths and `open`; do not run them on Linux.
Add `-- --render` to the inspection command only when a new preview is needed.
It writes `Art/CoastalSlice/encounter.png`; it does not replace the approved target.
Preserve the canonical scene before any rebuild: the full builder writes to its
existing output path. Use `--python-exit-code 1` for batch Python runs so errors
cannot masquerade as successful Blender process exits.

Recorded art checks: 4,402 objects, 47 materials, 1,302,916 evaluated triangles,
3 cameras, 12 lights, both guards, gorilla hands and banana charm present, and no
unpacked external image dependencies. These are source-art observations, not a
runtime budget approval. Dense foliage and 52,000 fur clumps need deliberate LOD,
material, draw-call, and shadow planning, not blind whole-scene export.

Once the official Lyra sample and Unreal installation are verified:

```bash
./Scripts/Lyra/build_and_play_baseline_linux.sh
```

The script discovers installations; inspect its resolver before setting optional
`UE_ROOT`/`LYRA_ROOT` overrides. Its initial window is 1600x900 and it launches
editor `-game`, not a packaged Shipping build. Neither that launch nor its FPS
counter satisfies the 1920x1080 packaged performance/acceptance gate by itself.

## Bounded Agent Responsibilities

Use the lead plus **at most two concurrent subagents**, not a growing agent tree.
These are focused AI roles, not a claim of human professional credentials.

| Owner | First bounded deliverable | Ownership |
| --- | --- | --- |
| Environment agent | Inspect/export one representative coastal module with baked PBR materials, collision plan, and measurable geometry/material budget. | Environment-only scripts and a separate export folder; no canonical scene overwrite. |
| Character agent | One skinned guard with tested weights and a small named idle/move/fire/react clip set; document remaining animation quality issues. Then tackle gorilla weapon-contact rigs. | Character-only scripts and separate character output folder; no environment changes. |
| Lead | Linux preflight, official Lyra baseline, integration, gameplay/AI, test evidence, Git and handoff updates. | Shared scene assembly, configuration, canonical .blend, README and integration files. |

Before dispatch, give each agent exact files, input/output paths, one completion
condition, and required checks. Assign shared files to one writer. Use separate
outputs or worktrees, not concurrent saves of the same binary `.blend`. Subagents
do not spawn agents, push branches, install dependencies, or expand scope without
the lead. Require a compact result: files changed, tests run, failures, next step.
Review and integrate one completed asset package before commissioning the next.

## Credit And Compute Discipline

- Default to Astra Medium for complex coordinated work. Raise reasoning only for
  a specific difficult problem. Use a lighter available model for well-scoped
  edits/checks when appropriate; do not force unavailable model IDs or assume
  subscription prices from API prices. Do not change the user's model silently.
- Start with one lead preflight, not two agents independently inventorying the
  same machine. Delegate only independent work that is ready to execute.
- Read targeted files and concise diffs/log tails. Do not reread the entire chat,
  repository, or large logs each turn. Save decisions in Git as they are made.
- Use low-resolution, low-sample previews and isolated asset tests first. Render
  full quality only for a candidate that passes cheaper checks. Reuse geometry,
  baked assets, engine caches, and incremental builds; never purge caches by habit.
- Check the first real compile/import error. After two failed attempts at the same
  fix, stop blind retries, isolate the cause, and revise the approach or request
  the missing evidence. Do not create another full package to test a one-line fix.
- Run each expensive render/build once and collect its exit status and useful
  logs. Wait with bounded tool calls instead of tight polling, repeated screenshots,
  or continually narrating unchanged progress.
- Each agent gets one asset-sized task and one validation pass. Further iterations
  need an identified defect, not vague requests to "make it more AAA."
- End each work package with a checkpoint: actual deliverable, measured checks,
  unresolved blocker, and next exact action. Continue to the next justified package
  while budget permits, not indefinitely just to exhaust usage.
- If usage status is available, check at the start and before another substantial
  package, not every tool call. At 20% or less remaining in either account window,
  finish a small safe checkpoint and avoid launching new long tasks. If unavailable,
  use the bounded packages above; do not invent a remaining-credit estimate.
- Do not redeem reset credits, purchase compute/assets, start paid hosting, or
  schedule automatic continuation without a fresh explicit request. No background
  task or resume-after-reset automation is established by this handoff.

These controls reduce wasted work; they cannot guarantee a fixed number of minutes
or credits. Blender/Unreal hardware time and model usage are different resources.
Preserve enough context for a new session to resume without paying to rediscover it.

## Acceptance And Next Milestone

Follow [Playability Gate](PLAYABILITY_GATE.md): official Lyra first, then one
Gorilla combat-room/encounter conversion, then a complete short coastal objective
loop. Do not build a full campaign, multiplayer service, new browser engine, or
cloud-streaming deployment during this milestone.

The first deliverable the user should judge is one small coastal encounter with
responsive movement/aim/fire/reload, an attacking guard, clear damage/death/restart,
Italian Bruno dialogue/subtitles, and a complete objective/extraction loop. Use
original or appropriately licensed assets and consented voice recordings. If a
required asset/recording is missing, identify it as missing, not production-ready.

Acceptance requires a local packaged build and actual testing: the controls and
AI work, the objective can be won or lost and restarted, and three ten-minute
sessions run without a blocker at the agreed 1080p/60 FPS target. Record hardware,
scalability, resolution, frame times and observed stalls. Retain the visual target's
GPU/thread budgets; negotiate any hardware-driven change rather than silently
lowering the bar. Compare a real gameplay capture with the reference. A compile,
editor launch, static render, or ability to walk around is not acceptance.

## Git And Continuity

Keep this as the single canonical Gorilla repository, with history preserved.
Use `codex/` branch names for further isolated work and Git LFS for large supported
asset types. Do not overwrite `main`, rewrite history, or delete archives as part
of setup. Merge only reviewed work; do not imply this branch is already deployed.

Before committing, inspect the scoped diff and scan new files for credentials,
account identifiers, personal paths, proprietary content, and unnecessary build
artifacts. Do not commit complete machine logs, Epic content, cache folders, or
cloud secrets. Push accepted checkpoints, verify the remote commit, and state any
push failure clearly. Never put tokens in scripts, docs, or command output.

At the end of each substantial package, update this handoff with the actual branch
and implementation commit, new assets, tests and failures, pending integration,
and the next bounded task. Keep local hardware/account details outside the public
repository. Do not label planned work as completed.
