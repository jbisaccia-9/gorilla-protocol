# Operazione Scimmia di Mare

The rebuild targets one authored ten-minute mission, not a broad framework. Agent
Bruno boards a storm-lashed superyacht off the Amalfi coast, steals an encrypted
ledger, and escapes aboard an undersized tender. The world treats the espionage
seriously; the absurdity comes from Bruno solving it confidently as a gorilla.

![Vertical-slice visual target](Art/vertical-slice-visual-target.png)

This image is a composition and quality target, not a production texture or a
claim that the corresponding 3D assets exist.

## Pillars

- **Gorilla, not a skin:** strength, reach, climbing, carrying, throwing, and
  noise materially change every encounter.
- **Improvised espionage:** stealth, firearms, traversal, and physical comedy
  remain viable before and after detection.
- **Readable action:** enemies communicate intent through pose, animation,
  muzzle direction, voice, and environment rather than floating labels.
- **Fast recovery:** detection changes the encounter; only death or falling
  restarts from a checkpoint.

## Mission Flow

| Time | Beat |
|---|---|
| `0:00-0:45` | Climb the anchor chain; teach movement and mantle in play. |
| `0:45-2:00` | Two-guard aft deck supporting ghost, lure, grab, or pistol. |
| `2:00-4:00` | Galley/lounge sandbox with three guards, camera, and radio operator. |
| `4:00-5:45` | Steal a keycard or use Knuckle Rush through a marked wall. |
| `5:45-6:30` | Recover the ledger from an overengineered banana sculpture. |
| `6:30-8:45` | Search or reinforcement escape based on alarm state. |
| `8:45-10:00` | Release the tender and leap aboard during a collapsing davit beat. |

## Signature Mechanics

- **Gorilla Grip:** tap to strike; hold to grab a valid staggered target. Carry,
  drop, and authored throws create knockdowns, distractions, or temporary cover.
- **Knuckle Rush:** sprint into a stable-camera quadrupedal charge that breaks
  marked barriers and knocks guards aside, but is loud and disables firearms.
- **Primate Traversal:** fast mantles, pipes, hangs, and ledges create tactical
  routes rather than a separate climbing minigame.
- **Bruno Button:** tap for a contextual Italian quip; hold to deliberately lure
  guards from Bruno's position.
- **Banana Decoy:** three per mission. The first guard crossing the authored
  interaction slips and drops their weapon; it is not uncontrolled physics.

## Combat Contract

- Suppressed P9: 12 rounds, one headshot or three body shots on a watchman.
- Guard SMG: 24 rounds, effective close, deliberately difficult in long bursts.
- Maximum five active enemies and two simultaneous ranged attackers.
- Watchman, radio operator, and enforcer have distinct silhouettes and behaviors.
- First-engagement damage has at least 450 ms of readable anticipation.
- Reinforcements use visible access points and never spawn behind the player.
- Awareness is local: unaware, curious, searching, confirmed. Guards communicate
  knowledge through audible speech and radios, not a global psychic alert.

## Approval Bar

- No Engine primitive, proxy character, placeholder weapon, floating bark, or
  checker material is visible from any gameplay camera.
- The first encounter genuinely supports ghosting, distraction, grabbing, and shooting.
- Eight of ten first-time testers finish without help in a median 8-12 minutes.
- Eighty percent use Gorilla Grip and Knuckle Rush before minute four without a menu.
- Local packaged input-to-shot latency is below 50 ms and hit reaction starts
  within 100 ms.
- The target PC sustains 1080p60; Linux/L4 Stream60 sustains a 14 ms GPU p95.
- Every Bruno voice asset is a consented Italian performance with localized subtitles.
- Seven of ten testers rate fun at least `4/5`; six voluntarily replay another route.
- Pixel Streaming is tested separately and cannot substitute for local gameplay approval.
