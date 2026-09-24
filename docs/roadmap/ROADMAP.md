# DuplexEngine roadmap — Phases 10+

This is the sequel to the modernization work in Phases 0–9 (see the repo's git log and, if you
have access to it, this project's Claude memory file for that history — GLM math + SIMD/fixed
point, EnTT ECS, Dear ImGui editor plumbing, Jolt physics). Those are done and committed as of
2026-09-23 (`fdb6ff4` is the most recent).

This document scopes what comes next: Kay's original 18-item wishlist, plus a further research
pass (2026-09-23 addendum, see below) that found real gaps in the list and features already
half-paid-for by earlier phases. Everything is researched and ordered into one dependency
graph. It lives in the repo (not just in a memory store tied to one machine/session)
specifically so any Claude Code session opened against this checkout — on any machine, with or
without prior conversation history — can pick it up cold. **Read this whole file before
starting any phase below**, not just the section for the phase you're about to do: the "why
here" notes explain load-bearing decisions made in earlier phases that later phases depend on.

Nothing in this document has been implemented yet. Treat every phase below the same way Phases
0–9 were treated: confirm the scope with Kay before starting if anything looks ambiguous, do a
full inventory of the relevant code before touching it, verify claims (compile, run, screenshot,
diff) rather than assuming, and don't commit without being asked.

Phase numbers were renumbered in this pass to slot new phases into their correct dependency
position (e.g. shadow mapping has to sit right next to the renderer it shadows, not at the end
of the file) rather than bolted on after item 18's export/bundle phase. Nothing had been
implemented under the old numbers, so nothing was lost by renumbering.

## Kay's original list (2026-09-23)

1. The editor should become a scene editor for glTF scenes
2. A way to export/publish/bundle a game
3. Base components/parenting/transforming
4. Importing obj/gltf/glb/usd/stl into the scene
5. Material editor/creator (PBR, Unlit, Custom, Particle (with billboarding))
6. Shader editor/creator
7. Skybox
8. Additional features: raytracing/SSR/SSAO/RTAO/HBAO/VXAO/reflection spheres/planes/cubemaps
9. Support for displacement/SSS/POM/clear coat
10. Variable rate shading
11. Both deferred shading and forward shading
12. RTGI
13. Upscaling (DLSS, FSR, XeSS)
14. Transparency (order-independent transparency, alpha blending)
15. Post-processing effects (bloom, motion blur, depth of field)
16. Audio system (3D spatial audio, reverb zones, occlusion)
17. Physics system (rigid bodies, soft bodies, collision detection, constraints)
18. Animation system (skeletal animation, blend shapes, inverse kinematics)

## 2026-09-23 addendum: what a further research pass found

Kay asked for more research into what else would be a good addition. Two kinds of things came
out of that pass, both folded into the phase list below rather than kept separate:

**Real, load-bearing gaps the original list didn't mention.** Checked the actual code for each
before assuming: there is **no shadow mapping anywhere in this engine** (the only "shadow" hits
in the engine's own code are the BRDF's geometric self-shadowing term in `bsdfPixel.hlsl` —
microfacet math, not a shadow map), **no frustum/occlusion culling, LOD, or GPU instancing**
(the "cull" hits in the renderer backends are just the rasterizer's back-face cull-mode state),
**no logging framework** (grepped for one, found nothing), **no general-purpose job/task
system** (only Jolt's own internal one, scoped to physics), and **no scripting layer at all** —
every piece of gameplay logic has to be C++ recompiled. None of these were on Kay's list, but a
"genuinely cross-platform game engine" roadmap without shadows or a logging system is missing
fundamentals, not stretch goals — they're added below, sequenced where they actually belong.

**Features the engine has already half-paid for.** Two stood out:
- Phase 9 vendored **Jolt Physics**, whose bundled source already includes
  `Jolt/Physics/Character/CharacterVirtual.h` (a full kinematic character controller) and
  `Jolt/Physics/Ragdoll/Ragdoll.h` (native ragdoll support), plus wheeled/tracked vehicle
  constraints — none of that is on Kay's list, but it's sitting in the repo already, unused, and
  wrapping it is a small fraction of the effort of building it from scratch. Added as their own
  phases below.
- Phases 3 and 9 both invested specifically in **cross-platform bit-exact determinism** (fixed-
  point math, `CROSS_PLATFORM_DETERMINISTIC` Jolt, verified thread-count-independence via a
  bit-exact memcmp test). That kind of investment is usually made for exactly one reason in game
  engines: it's what **deterministic lockstep / rollback multiplayer** (GGPO-style) needs to
  work at all — every client simulates the same inputs and gets bit-identical results, so only
  inputs need to travel over the network. Networking wasn't on Kay's list, but the engine is
  unusually well-positioned for it already, and that's worth surfacing explicitly rather than
  leaving the determinism work's main payoff unscoped. Added as its own phase, flagged clearly
  since it's a genuinely large addition Kay should actively confirm wanting, not something to
  assume.

**Follow-up same day: an in-game GUI system**, explicitly requested by Kay with a specific
starting idea (HTML/JavaScript, "like some engines do it") and an explicit invitation to check
whether something else would serve better. Researched four real options (Coherent Gameface, CEF,
Ultralight, RmlUi) — see Phase 37 for the full comparison. Short version: literal HTML/JS is a
real, viable path (Ultralight specifically, not CEF — CEF's multi-process IPC model and
Windows-favoring OSR story are worse fits here), but the researched recommendation is **RmlUi**
instead — open source (matching every other dependency choice in this document), purpose-built
for games rather than adapted from a browser, with a first-party data-binding system that
directly answers Kay's "injecting values" ask, and reuses Phase 46's Lua scripting investment
instead of embedding a second, unrelated scripting runtime just for menus. Flagged for
confirmation like Phase 38/47, precisely because Kay's own starting framing (HTML/JS) points a
different direction than the research recommendation — worth an explicit yes/no rather than
assuming either answer.

**Second follow-up same day: lighting as its own phase, lightmap baking, and a feature-toggle
convention.** Kay asked for a separate lighting phase next to shadow mapping (point/area/
directional/cone lights), lightmap baking, and a general note that most features should be
enable/disable-able via CMake or in the editor. Checked the code before scoping: the existing
`Light` component already covers spot/directional/point (spot and "cone" are the same thing —
area lights were the real gap), but the bigger finding was that the light *array* is hardcoded
to 8 lights with no culling at all (`shadercb.h`'s `MAX_LIGHTS`) — added as Phase 20, ahead of
shadow mapping (Phase 21) since shadow technique depends on which light types exist. Lightmap
baking is added as Phase 31, positioned to reuse Phase 29's raytracing acceleration structures
rather than adding a second raytracing dependency, and reuses an already-unused `isStatic` flag
that's been sitting on `Transform` since Phase 11. The feature-toggle convention is written up
once as its own section (see "Feature toggles" below) rather than repeated in full per phase,
with short pointers added to the phases it matters most for.

**Third follow-up same day: a sample project.** Kay asked for a final phase — a showcase scene,
generated via CMake, built from open-source models, exercising most/all of the engine's
features. Added as Phase 49. Researched real asset sources rather than gesturing at "some open
models": Khronos's `glTF-Sample-Assets` repo for PBR/animation/GI test content (with specific
models picked for specific phases, not just "grab some"), Poly Haven for CC0 HDRIs, Kenney.nl
for CC0 audio/UI. Found a genuine, documented licensing gotcha in the process — a past version
of Sponza in that exact Khronos repo violated Crytek's license terms (their issue #172) — which
turned "remember to check licenses" from boilerplate advice into a scoped requirement (fetch
each model's own README/license file, ship a consolidated `CREDITS.md`). Also flagged that this
shouldn't actually be built only at the end despite being the last phase in the document —
starting it early and growing it alongside later phases makes it useful as a running integration
check instead of a checkbox exercise after everything else is done.

## Phase index

| # | Phase | New in this pass? |
|---|---|---|
| 10 | Logging system | new |
| 11 | Base components: hierarchy/parenting/transforming | — |
| 12 | Serialization: scene format, save games, prefabs | expanded |
| 13 | Asset/resource management | new |
| 14 | Importing OBJ/glTF/GLB/USD/STL | — |
| 15 | Editor becomes a real glTF scene editor | expanded |
| 16 | In-editor profiler / debug overlay | new |
| 17 | D3D12: a real backend | done 2026-09-23 |
| 18 | Job/task system | new |
| 19 | Deferred + forward hybrid rendering architecture | — |
| 20 | Lighting: point/area/directional/spot lights | new |
| 21 | Shadow mapping | new |
| 22 | Anti-aliasing (TAA/MSAA/FXAA) | new |
| 23 | Skybox / IBL | — |
| 24 | GPU/CPU culling, LOD, GPU instancing | new |
| 25 | Shader editor/creator | — |
| 26 | Material editor/creator | — |
| 27 | Displacement, SSS, POM, clear coat | — |
| 28 | Screen-space effects: SSAO/SSR/HBAO + decals | expanded |
| 29 | Hardware raytracing foundation | — |
| 30 | RTGI | — |
| 31 | Lightmap baking | new |
| 32 | Variable rate shading | — |
| 33 | Order-independent transparency | — |
| 34 | Particle system / VFX | new |
| 35 | Post-processing: bloom, motion blur, DoF, color grading | expanded |
| 36 | Upscaling (DLSS/FSR/XeSS) | — |
| 37 | In-game GUI system: HTML/CSS UI + value injection | new, flagged |
| 38 | Environment rendering: terrain, water, volumetric fog | new, optional |
| 39 | Audio system | — |
| 40 | Physics: soft bodies, constraints, collision events | — |
| 41 | Character controller (+ optional vehicle physics) | new |
| 42 | Ragdoll physics | new |
| 43 | Navigation & pathfinding | new |
| 44 | Animation: skeletal, blend shapes, IK | — |
| 45 | Animation state machines / blend trees | new |
| 46 | Scripting system | new |
| 47 | Networking: deterministic lockstep / rollback multiplayer | new, flagged |
| 48 | Export / publish / bundle | — |
| 49 | Sample project: CMake-generated showcase from open-source assets | new |

## One thing not on the list, found while researching it (resolved 2026-09-23 — see Phase 17)

**`driver.d3d12` used to be a scaffold, not a working renderer** — every method was a stub. This
mattered because several phases below (raytracing, variable rate shading, and to a lesser extent
upscaling) are APIs D3D11 structurally cannot expose (no DXR, no VRS caps, no mesh shaders) — on
Windows they need D3D12 specifically. Vulkan already has all the relevant extensions on both
Windows and Linux.

Rather than block every advanced-rendering phase on a full D3D12 implementation, the plan below
still targets **Vulkan as the first-class backend for every new rendering feature**, with D3D12
as an explicit follow-up per feature (this is unchanged even now that Phase 17 is done — D3D12
existing doesn't mean later phases should build against it first). D3D11 keeps working for
everything that doesn't need the newer hardware capabilities, and gracefully lacks the rest
(capability-checked, not silently broken).

## Feature toggles: CMake build options vs. editor/runtime settings

Kay's note: most features added from here on should be possible to disable/enable — either via
CMake or in the editor. Worth being explicit that these are two different mechanisms serving two
different purposes, not one thing with two names, and both should be considered from the start
of a phase rather than bolted on afterward:

- **CMake build-time options** (`option()`, following the existing `CACHE BOOL "" FORCE` pattern
  this project already uses for `CROSS_PLATFORM_DETERMINISTIC`/`USE_AVX`/`USE_AVX2` in the
  top-level `CMakeLists.txt` for Phase 9's Jolt integration — this isn't a new convention, it's
  extending one already in the repo) control **what gets compiled and linked at all**. Use these
  for anything that pulls in a real dependency or meaningfully affects binary size/footprint:
  raytracing, RTGI, upscaling (Streamline + per-vendor SDKs), the GUI library (RmlUi/Ultralight),
  scripting (sol3/Lua), networking, lightmap baking, environment rendering. A build that doesn't
  want a feature shouldn't need to link its dependency at all — e.g. a headless/server-style
  build wants `DUPLEX_ENABLE_GUI=OFF` and `DUPLEX_ENABLE_UPSCALING=OFF` without those libraries
  even appearing in the link step. Name these consistently, e.g. `DUPLEX_ENABLE_RAYTRACING`,
  `DUPLEX_ENABLE_UPSCALING`, `DUPLEX_ENABLE_GUI`, `DUPLEX_ENABLE_SCRIPTING`,
  `DUPLEX_ENABLE_NETWORKING`.
- **Editor/runtime settings** control **what's active for a given project or quality tier**,
  among whatever a given build actually compiled in. This is where per-project render-quality
  toggles belong (SSAO/SSR on or off, shadow resolution, VRS on or off, particle density caps,
  which upscaler is selected) — exposed once Phase 16's profiler/debug-overlay panel and Phase
  15's editor settings UI exist to host them.
- **These two connect to the capability-query pattern already established for hardware-gated
  features** (Phase 29/32's `Renderer::SupportsRaytracing()`-style queries, needed because D3D11
  structurally can't do raytracing/VRS at all). A runtime toggle should never offer a feature the
  current build didn't compile in (CMake gate) or the current GPU/backend can't run (capability
  query) — the editor UI should read both and grey out/hide accordingly, not maintain a third,
  separate notion of "is this available."

Not every phase needs both mechanisms — foundational architecture (hierarchy, serialization,
the deferred+forward renderer itself, logging) isn't really "a feature to toggle," it's what
everything else is built on. This applies most to the phases that are genuinely optional or
heavy: raytracing and everything downstream of it, VRS, upscaling, the GUI system, scripting,
networking, lightmap baking, and environment rendering.

## Dependency graph (informal)

```
Phase 10 (logging)  — no dependencies, useful immediately, do first

Phase 11 (hierarchy/parenting)
  ├─→ Phase 12 (serialization: scenes/saves/prefabs — prefabs are serialized sub-hierarchies)
  ├─→ Phase 13 (asset/resource management)
  │     └─→ Phase 14 (importers: obj/gltf/glb/stl, usd flagged separately)
  │           ├─→ Phase 15 (scene editor)  [also needs Phase 12 for scene save]
  │           └─→ Phase 44 (animation — skeletons are joint hierarchies + gltf is the anim source)
  └─→ (feeds world-transform computation used by every system below)

Phase 15 (scene editor) ─→ Phase 16 (profiler/debug overlay hosted in the editor's ImGui panels)

Phase 17 (D3D12: real backend) ──→ unlocks D3D12 leg of Phases 29/30/32/36

Phase 18 (job/task system) ─→ Phase 24 (culling/LOD/instancing), Phase 34 (particles), touches
                               animation batch sampling in Phase 44

Phase 19 (deferred + forward hybrid renderer)
  ├─→ Phase 20 (lighting: point/area/directional/spot — the light model shadows/screen-space
  │     effects/RTGI all build on)
  │     └─→ Phase 21 (shadow mapping — per-light-type shadow technique needs the light types
  │           this phase defines to already exist)
  ├─→ Phase 22 (anti-aliasing — wants Phase 19's motion vectors)
  ├─→ Phase 23 (skybox / IBL ambient)
  ├─→ Phase 24 (culling/LOD/instancing — needs Phase 18)
  ├─→ Phase 25 (shader editor) ─→ Phase 26 (material editor) ─→ Phase 27 (displacement/SSS/POM/clearcoat)
  ├─→ Phase 28 (SSAO/SSR/HBAO/decals — screen-space, no new hardware caps needed)
  │     └─→ Phase 29 (hardware raytracing foundation: RTAO, true reflections, probes)
  │           ├─→ Phase 30 (RTGI)
  │           └─→ Phase 31 (lightmap baking — reuses Phase 29's acceleration structures/ray
  │                 dispatch to bake static lighting offline instead of at runtime)
  ├─→ Phase 32 (variable rate shading)
  ├─→ Phase 33 (order-independent transparency)
  ├─→ Phase 34 (particle system — needs Phase 18 + Phase 26's particle material type)
  ├─→ Phase 35 (post-processing — wants Phase 19/22's buffers)
  ├─→ Phase 36 (upscaling — wants Phase 19/22's motion vectors + jitter)
  │     └─→ Phase 37 (in-game GUI — composites after upscaling, see its own "why here")
  └─→ Phase 38 (environment rendering — optional, confirm scope first)

Phase 37 (in-game GUI)                   — flagged, confirm HTML/JS vs. RmlUi with Kay first;
                                            loosely wants Phase 46 (scripting) for UI event logic
Phase 39 (audio)                         — independent, good parallel-track candidate
Phase 40 (physics: soft bodies/constraints) — extends Phase 9's Jolt integration directly
  ├─→ Phase 41 (character controller — Jolt's CharacterVirtual, already vendored)
  ├─→ Phase 42 (ragdoll — Jolt's native Ragdoll, needs Phase 44 too)
  └─→ Phase 43 (navigation/pathfinding — needs Phase 14 for level geometry to bake)
Phase 44 (animation) ─→ Phase 45 (animation state machines/blend trees)
Phase 46 (scripting)                     — independent, but more useful once 11/12 exist
Phase 47 (networking)                    — independent of rendering, wants 12 (state (de)serialization)
Phase 48 (export/bundle)                 — deliberately last
```

---

## Phase 10 — Logging system

**Depends on:** nothing. Do this first — every phase after it benefits from having real logs
instead of ad-hoc `fprintf`/`[DBG]` breadcrumbs (which earlier phases already had to add and
then manually strip — see Phase 5/9's history of exactly that).

**Scope:** **spdlog** — header-only (in its simplest usage), no dependencies, cross-platform
(Windows/Linux/macOS), fast enough for a hot path if ever needed (async mode via lock-free
queues), sane leveled/categorized logging with sinks (console with color, rotating file). Wire
a category per subsystem (`Renderer`, `Physics`, `Audio`, `ECS`, `Editor`, ...) so log output
can be filtered per-system once there's enough of it to need filtering. Route it into the Phase
16 in-editor debug overlay once that exists, as a log panel.

**Where it lives:** new `engine.log` module (or fold into `engine.shared/` given how small this
is — decide when the phase starts), `Duplex::Log` namespace.

---

## Phase 11 — Base components: hierarchy, parenting, transforming

**Depends on:** nothing (first content-facing phase).

**Why here:** Checked the actual code — [transform.h](../../engine.core/ecs/components/transform.h)
today is flat: `position`/`rotation`/`scale`, no parent field, no concept of local-vs-world
space. Every later phase that needs a scene graph (the editor becoming a real scene editor,
glTF import — glTF scenes are node hierarchies by spec, skeletal animation — skeletons are joint
hierarchies) needs this to exist first.

**Scope:**
- Split `Transform` into a `LocalTransform` (what exists today) and a computed `WorldTransform`
  (cached world matrix), or add `entt::entity parent = entt::null;` plus a cached
  `Mat4x4 worldMatrix` to the existing component. The split is recommended — keeps "local" and
  "derived" data clearly separated, matching how `ToShaderLayout()` already treats the
  shader-facing matrix as a derived view, not the source of truth.
- A `TransformSystem::Update(registry)` walking entities in parent-before-child order, composing
  `worldMatrix = parentWorld * local`. EnTT doesn't give topological order for free — either
  maintain an explicit depth/dirty-flag scheme, or do the simple thing first (recompute all,
  sorted by an explicit pass) and only optimize if profiling (Phase 16) says so.
- Update every consumer of `Transform` (camerasystem.h, meshsystem.h, lightsystem.h,
  physicssystem.h) to use the computed world matrix — full call-site inventory before touching
  anything, same discipline as every prior phase.
- Reparenting API preserving world transform by default (recompute local from the new parent's
  world inverse) — the behavior any scene/DCC tool user expects when dragging a node onto a new
  parent in an outliner.

**Where it lives:** `engine.core/ecs/components/transform.h` (extend), new
`engine.core/ecs/systems/transformsystem.h`.

**Open question for Kay:** cycle detection on reparenting (A parented to its own descendant) —
hard assert in debug builds, silent no-op in release, or something else? Flag when this phase
starts rather than guessing.

---

## Phase 12 — Serialization: scene format, save games, prefabs

**Depends on:** Phase 11 (there's nothing meaningful to serialize without a component set and a
hierarchy).

**Why here, and why generalized beyond "scene save":** the original scoping of the scene editor
phase (now Phase 15) flagged "scenes need a serialization format" as a loose sub-decision. That
format shouldn't be designed twice — a **prefab is a serialized sub-hierarchy** (a template that
can be instantiated multiple times), and a **save game** is serialized gameplay-relevant
component state at a point in time. All three want the same underlying capability: walk an
EnTT registry (or a subset of it), (de)serialize each component's data. Doing this once, with
enough generality to cover all three, avoids three separate ad-hoc formats.

**Scope:**
- A per-component (de)serialize hook — simplest viable approach to start: a manually maintained
  registration table (`RegisterSerializable<Transform>(...)`) rather than full reflection;
  reflection is a nice-to-have, not a blocker, same call already made for Phase 15's inspector
  panel.
- File format: JSON (human-readable, diffable, good for scenes/prefabs authored/reviewed by
  humans) vs. a simple binary format (faster, smaller, better for save games). Recommend JSON
  for scenes/prefabs, a separate lean binary format for save games — they have different
  priorities (diffability vs. load speed/size) and don't need to share a format just because
  they share the underlying (de)serialize mechanism.
- Prefab instancing: spawning a prefab creates a new sub-hierarchy under the Phase 11 parenting
  system, with per-instance overrides (position at minimum) tracked separately from the prefab's
  own stored defaults.

**Where it lives:** new `engine.core/serialization/` (or `engine.serialization` module).

---

## Phase 13 — Asset/resource management

**Depends on:** Phase 11/12 loosely; mainly a prerequisite for Phase 14 (importers) — see why.

**Why here:** today, texture/mesh loading is direct and synchronous (`Renderer::CreateTexture`,
`MeshRenderer::SetMesh` immediately calling into the renderer) with no reference counting, no
async I/O, and no hot-reload. That's fine for the current hand-authored demo content, but
building Phase 14's importers directly against that ad-hoc pattern means redoing the loading
path a second time later when async/streaming/hot-reload inevitably becomes necessary. Doing
this once, before importers exist, means importers get built against the real interface from
the start.

**Scope:**
- Handle-based resource cache (`AssetHandle<Texture>`, `AssetHandle<Mesh>`, ...) with reference
  counting — mirrors the existing `HandlePool`/generation-checked-handle pattern already used
  for GPU resources in the renderer backends, applied one layer up at the asset level.
- Async loading (background thread(s) — natural pairing with Phase 18's job system once that
  exists, though a minimal single-background-thread version can ship before Phase 18 lands if
  sequenced that way instead).
- Hot-reload: watch imported asset files, reload+swap the underlying GPU resource without an
  entity losing its handle. Directly useful for Phase 25's shader-editor "compile on save" note
  and for iterating on imported meshes/textures without restarting.

**Where it lives:** new `engine.core/resources/assetmanager.h` (or promote to its own
`engine.assets` module if it grows large enough to warrant it).

---

## Phase 14 — Importing OBJ / glTF / GLB / USD / STL

**Depends on:** Phase 11 (imported scenes are node hierarchies), Phase 13 (imported resources
should be created through the asset manager, not ad hoc).

**Scope, per format:**
- **glTF / GLB** (primary — this is what the Phase 15 scene editor should be built around, per
  Kay's item 1): **tinygltf** (header-only, C++11, permissive license, the most widely used
  glTF loader in hobbyist/indie C++ engines). glTF's `pbrMetallicRoughness` material maps almost
  1:1 onto [bsdfmaterial.h](../../engine.core/graphics/bsdfmaterial.h)'s existing base-color/
  metallic/roughness/normal/emissive slots.
- **OBJ**: **tinyobjloader** — header-only, same ecosystem as tinygltf. Flat format, one
  implicit root, no hierarchy of its own — imports as a single node under wherever it's dropped.
- **STL**: no library needed — a ~150-line hand-rolled reader covers both binary and ASCII STL
  (a triangle soup, no materials, no hierarchy, not even indexing).
- **USD**: flagged, not fully scoped. Pixar's OpenUSD is large and heavy compared to every other
  dependency in this project (GLM/EnTT/Jolt are all small and focused). **Revisit with Kay when
  this phase starts**: is full OpenUSD a hard requirement, or does "author in a DCC tool, export
  to glTF" cover the real use case? If full USD import really is required, it's arguably its own
  phase rather than a bullet in this one.

**Where it lives:** new `engine.core/resources/import/` (or `engine.import` module, matching
the `engine.X` naming convention).

---

## Phase 15 — The editor becomes a real glTF scene editor

**Depends on:** Phase 11 (hierarchy), Phase 12 (scene save format), Phase 13 (asset handles for
whatever gets dropped in), Phase 14 (import), Phase 7 (already done — ImGui plumbing exists).

**Scope:**
- Outliner panel: tree view of the registry via Phase 11's parent/child links, drag-and-drop to
  reparent.
- Inspector panel: editable ImGui widgets per component on the selected entity.
- **Undo/redo**: a command-pattern stack where every editor mutation (move, reparent, add/remove
  component, property edit) is represented as an undoable command. This wasn't on Kay's original
  list but is close to a hard requirement for any editor people actually use — worth building in
  from the start rather than retrofitting after enough mutation call sites exist that wrapping
  them all becomes its own project.
- **Asset/content browser panel**: thumbnails, folder navigation, search over whatever Phase 13
  tracks — the natural complement to the outliner (browsing what's *available* vs. what's *in
  the scene*), also not on the original list but a standard, expected editor panel.
- Viewport gizmos for translate/rotate/scale — **ImGuizmo** (MIT-licensed, the standard
  ImGui-native companion for exactly this) rather than hand-rolled gizmo math.
- Drag-and-drop import routed through Phase 14's importers.
- Scene save/load through Phase 12's format.

**Where it lives:** `engine.editor/` (extends Phase 7's work directly).

---

## Phase 16 — In-editor profiler / debug overlay

**Depends on:** Phase 15 (needs an editor with ImGui panels to render into), Phase 10 (surfaces
log output too).

**Why here, and why it wasn't on the original list:** every phase from here on adds more moving
parts (deferred passes, raytracing, particles, animation batches) that are hard to reason about
without frame-timing/GPU-timing visibility. This is a standard engine dev-tool, cheap relative
to its payoff, and better added once (now, while the feature surface is still small) than
retrofitted once there's a lot to instrument.

**Scope:** CPU frame-time graph, per-pass GPU timing (Vulkan timestamp queries; D3D11/D3D12
equivalents), draw-call/triangle counters, a log panel (Phase 10's output), basic memory usage
(GPU resource counts via the existing `HandlePool`s' alive-counts — cheap, already tracked).

**Where it lives:** `engine.editor/` (new panel), thin instrumentation hooks added into
`driver.vulkan/`/`driver.d3d/`/`driver.d3d12/` alongside existing pass code.

---

## Phase 17 — D3D12: a real backend, not a scaffold (done, verified 2026-09-23)

**Depends on:** nothing structurally, but gates the D3D12 leg of Phases 29 (raytracing), 32
(VRS), and partly 36 (upscaling) — see "One thing not on the list" above.

**Scope:** brought `driver.d3d12` to the same feature level `driver.d3d` (D3D11) and
`driver.vulkan` already reached — device/swapchain/pipeline/descriptor-heap setup, the same
BSDF/PBR shader pipeline the other two backends run, ImGui wired in. No new engine-facing
features, just parity. Device/swapchain/RTV/DSV setup follows D3D11's shape; frame
synchronization (per-frame command allocators, a fence, `MAX_DRAWS_PER_FRAME`-budgeted per-draw
constant/descriptor slices) follows Vulkan's shape instead, since D3D12 — like Vulkan and unlike
D3D11's immediate context — needs the caller to manage CPU/GPU overlap explicitly. Root
signature: two root CBVs (b0/b1) plus an SRV descriptor table (t0-t5) and two static samplers
(s0/s1, so no sampler heap is needed at all). Verified by actually running `DuplexEngine.exe`
with a `DUPLEX_RENDERER=d3d12` opt-in env var (added to `engine.client/client.cpp`, default
stays D3D11) and screenshotting the live window: BoomBox model, physics-driven box stack,
lighting, and texture sampling (the boombox's "PLAY" button, visibly rotating frame to frame) all
confirmed rendering correctly, matching D3D11's output.

**Two real, pre-existing bugs found and fixed along the way — not scoped to D3D12 alone, since
neither backend had ever actually been *run* on Windows before this (only compiled; Phase 9 was
verified on Linux only):**
- **Both D3D11 and D3D12's shared shader cross-compile path (SPIRV-Cross → HLSL) was completely
  broken.** `res/CMakeLists.txt` compiles the shared `.spirv` with `-fvk-t-shift 10 0 -fvk-s-shift
  20 0`, purely so Vulkan's textures/samplers don't collide with cbuffers b0/b1 in one descriptor
  set. SPIRV-Cross's HLSL backend preserves those shifted binding numbers verbatim as HLSL
  register numbers unless told otherwise, which both overshoots HLSL SM5.0's 16-slot sampler
  limit (`s20` doesn't exist — a hard `D3DCompile` error, confirmed directly) and disagrees with
  where each renderer actually binds resources (t0../s0..). Fixed with a small
  `RemapVulkanShiftedBindingsToHlsl()` helper (duplicated in both `d3d11renderer.cpp` and
  `d3d12renderer.cpp`, matching those files' existing non-shared-code convention) that walks
  `get_shader_resources()` and calls `add_hlsl_resource_binding()` per resource to undo the
  shift — the nonobvious part being that the remap has to be keyed with `.stage =
  compiler.get_execution_model()` explicitly, since it defaults to `ExecutionModelMax` and the
  lookup uses the compiler's real stage, so an unset `.stage` silently never matches. D3D11 was
  also missing a `return` after a failed `D3DCompile`, so the pre-fix failure crashed instead of
  failing cleanly — fixed alongside. `pixelSDFDefault.hlsl` (dead code — never bound by any
  backend, confirmed by grep) has its own separate, unrelated register-binding gap (no explicit
  `register()` declarations at all) and was simply dropped from D3D11's `CreateShader()` instead
  of chased, matching Vulkan's reference behavior of never loading it either.
- **A real D3D12-specific bug**, found via GPU-based validation after the above fix still left
  D3D12 hanging (`DXGI_ERROR_DEVICE_HUNG`/TDR) a few frames in: `engine.core`'s `Texture2D`
  (`texture2d.cpp`, shared code) creates a texture, creates its SRV, then immediately releases
  the texture — relying on `ID3D11ShaderResourceView` automatically holding a COM reference to
  its source resource, which D3D11 does but D3D12 descriptors do not (a D3D12 descriptor is just
  metadata written into a heap slot; releasing the app's only reference actually destroys the
  resource out from under it). Fixed entirely inside `D3D12Renderer` rather than touching the
  shared `texture2d.cpp` lifetime contract: `CreateTextureSRV`/`CreateCubemapSRV` now take an
  explicit extra `AddRef()` on the resource, released by `ReleaseTextureSRV` — reproducing
  D3D11's real lifetime semantics locally. **Confirmed Vulkan has the identical bug** (see the
  same-day Vulkan verification pass below) — flagged here, then actually fixed there.
- Separately, unrelated to shaders: the top-level `CMakeLists.txt` never disabled Jolt's own
  `JPH_USE_DX12`/`JPH_USE_VK`/`JPH_USE_MTL`/`JPH_USE_CPU_COMPUTE` options (all default `ON` in
  Jolt's `Build/CMakeLists.txt`), which pulled in an entirely unused experimental GPU-compute
  (hair simulation) module with its own shader compile step and its own `find_package(Vulkan)`
  call. This broke the Windows build outright — both locally (a warning-as-error against the
  newer Windows SDK, plus a broken relative `-I` path in Jolt's own dxc invocation) and in CI
  (the bogus `VULKAN_SDK/include` path GitHub Actions hit). Fixed by forcing all four flags `OFF`
  in the top-level `CMakeLists.txt`, matching the existing `CACHE BOOL "" FORCE` pattern already
  used for `USE_AVX`/etc.

**Where it lives:** `driver.d3d12/` (`d3d12renderer.h`/`.cpp`), `driver.d3d/d3d11renderer.cpp`
(the shared shader-binding-remap fix), `engine.imgui/CMakeLists.txt` (added the
`imgui_impl_dx12` backend sources), `engine.client/client.cpp` (the `DUPLEX_RENDERER=d3d12`
opt-in used to verify it).

### Same-day follow-up: Vulkan verified on Windows too, two more real bugs found and fixed

Kay asked for `driver.vulkan` to be verified on Windows as well (it had the same problem as
D3D11/D3D12 going in: compiled, never actually run). Added a `DUPLEX_RENDERER=vulkan` opt-in
alongside the `d3d12` one (both read before `window.Init()` now, since Vulkan needs
`SDL_WINDOW_VULKAN` set at window-creation time, not after). Two more real, pre-existing bugs
found, both root-caused by tracing exactly where in `Init()`/the render loop things went wrong
rather than guessing:

- **Dangling pointers to loop/if-block-scoped locals in `CreateLogicalDevice()` and
  `CreateSwapchain()`.** `CreateSwapchain()`'s `VkSwapchainCreateInfoKHR::pQueueFamilyIndices`
  was set to point at a `ui32 queueFamiliyIndices[]` array declared *inside* the `if` block that
  sets it, which goes out of scope before `vkCreateSwapchainKHR` — the actual reader — runs later
  in the same function. On hardware where the graphics and present queue families genuinely
  differ (this machine's RTX 3090: graphics family 0, present family 2 — exercising a branch that
  apparently never got tested before), this produced exactly the symptom the Vulkan validation
  layer flagged: garbage queue family index values reaching the driver, then a crash shortly
  after. `CreateLogicalDevice()`'s `VkDeviceQueueCreateInfo::pQueuePriorities` had the identical
  shape (pointing at a `float queuePriority` re-declared fresh each loop iteration) - fixed
  alongside even though it happened not to be the one causing visible symptoms this time, since
  it's the same latent bug.
- **The Vulkan pipeline's vertex input state was missing `bitTangent` (location 4).** A comment
  here claimed DXC strips it as unused and so it didn't need a binding — contradicted directly by
  Vulkan's own validation layer on real hardware ("Vertex shader consumes input at location 4 but
  not provided"), which then crashed the pipeline. Fixed by adding the location-4 attribute,
  matching D3D11Renderer/D3D12Renderer's input layouts, which both already included it (the
  comment's claim was simply wrong, not something that used to be true and later changed).
- **Confirmed the texture-lifetime bug flagged (but not fixed) under Phase 17 above.**
  `VulkanImageView` (a `VkImageView`) held no reference to its source `VkImage` either, so
  `texture2d.cpp`'s create-SRV-then-immediately-release-the-texture pattern destroyed the image -
  and freed its VMA-managed memory - while views still pointed at it; segfaulted reliably a few
  texture loads after `Init()` returned. Vulkan has no automatic COM-style refcounting to lean on
  the way the D3D12 fix did, so this one uses a `std::shared_ptr<void>` whose deleter calls
  `vmaDestroyImage`, copied into both the texture pool entry and every `VulkanImageView` created
  from it - the image is destroyed once the last copy (in either pool) goes away, regardless of
  release order. One real ordering subtlety this introduced: that deleter captures the
  `VmaAllocator` by value, so every pool holding a copy of the `lifetime` shared_ptr must be
  drained *before* `vmaDestroyAllocator()` runs in `Shutdown()` - not left to whenever
  `VulkanRenderer`'s own implicit destructor happens to run (well after `Shutdown()` returns,
  too late). `Shutdown()` now explicitly resets both pools before destroying the allocator.

**Also fixed while in this code, same root cause across all three backends:** `CreateShader()`
(D3D11, D3D12) and `CreateShaderModuleFromFile()` (Vulkan) all `throw std::string(...)` on a
missing/unreadable shader file, and nothing anywhere catches it — an entirely ordinary,
easy-to-hit mistake (running the built .exe with the wrong working directory - e.g. double-
clicking it in Explorer, which sets the working directory to the exe's own folder, not the repo
root the `./bin/data/shd/...` paths assume) crashed the process silently, with no error dialog
and no diagnosable message. Confirmed directly: this is exactly what a report of "a window opens
and closes immediately" turned out to be. Fixed by converting these to the same
`MessageBoxA`(Windows)/`fprintf(stderr, ...)`(Vulkan, which also builds on Linux) + clean-return
convention already used everywhere else in these files, with a message that says outright what's
wrong and how to fix it. Other, less commonly-hit `throw std::string(...)` sites earlier in
`VulkanRenderer::Init()` (missing validation layer, no supported physical device, surface
creation failure) were left as-is - genuine environment/driver problems, not the "wrong working
directory" failure mode actually reported, and not something this pass went looking for.

**Where it lives:** `driver.vulkan/vulkanrenderer.cpp`/`.h`, `driver.d3d/d3d11renderer.cpp` and
`driver.d3d12/d3d12renderer.cpp` (the shader-file-error-handling fix), `engine.client/client.cpp`
(the `DUPLEX_RENDERER=vulkan` opt-in).

### Second same-day follow-up: "resizing does nothing" and "Vulkan renders upside down"

Two more reports, both reproduced and fixed:

- **Vulkan rendered every frame upside down.** `camerasystem.h` builds the projection matrix via
  `glm::perspective`, shared unmodified across all three backends - GLM follows OpenGL's clip-
  space convention (Y+ up), which happens to already match D3D's, but not Vulkan's (Y+ down by
  default). This is the well-known "Vulkan Y-flip" - confirmed directly by the fact that D3D11
  and D3D12 rendered correctly with the exact same shared camera math, and only Vulkan didn't.
  Fixed entirely inside `VulkanRenderer::BeginScene()` with a negative-height viewport (core
  since Vulkan 1.1, no extension needed at the `VK_API_VERSION_1_2` this project already
  targets) - no shared-code changes. **`frontFace` needed no change to match** - see the third
  same-day follow-up below for why an initial attempt to also flip it turned out to be wrong.
- **Resizing the window didn't change what was rendered.** Two separate things were actually
  true here, and it's worth being precise about which is which: the swapchain/backbuffer itself
  *did* resize correctly on all three backends (verified directly - both a programmatic resize
  and a real interactive drag-resize, on D3D11, D3D12, and Vulkan, all correctly filled the new
  window size with no stretching or black bars, matching Kay's report about content not visually
  updating mid-drag as expected default behavior for a plain SDL app, not a bug). What was
  actually broken: `camerasystem.h`'s projection matrix used a **hardcoded 16:9 aspect ratio**
  (`1.7777f`), completely independent of the window's actual size - and structurally had no way
  to be anything else, because `Renderer` (`driver.graphics/renderer.h`) exposed no public
  accessor for its own current width/height at all. So the backbuffer resized, but the camera's
  framing never reflected it - which is a very reasonable thing to describe as "resizing does
  nothing." Fixed by adding `Renderer::GetWidth()`/`GetHeight()` (backed by the `width`/`height`
  members every backend's `Init()`/`Resize()` already keeps current) and computing the real
  aspect ratio in `camerasystem.h` from those. Verified: resizing to a window shape far from
  16:9 (an extreme tall/narrow window) now visibly changes the camera's framing to match, instead
  of staying pinned to the old aspect.

**Where it lives:** `driver.vulkan/vulkanrenderer.cpp` (viewport + winding fix),
`driver.graphics/renderer.h` (`GetWidth()`/`GetHeight()`), `engine.core/ecs/systems/
camerasystem.h` (real aspect ratio).

### Third same-day follow-up: side-by-side comparison, "D3D12/Vulkan boxes are grey but D3D11's are textured", "Vulkan has flipped culling"

Kay asked for all three backends to be launched together and screenshotted at the same moment
for a fair comparison (earlier screenshots in this document were taken at different times, which
matters here: the demo's BoomBox rotates and its physics box stack keeps falling/settling on
`Time::deltaTime`, both independent per-process clocks - confirmed by launching two instances of
the *same* backend side by side and seeing them already drift apart by a small amount). Kay then
directly spotted two real, genuine cross-backend discrepancies in the comparison - both
reproduced and fixed:

- **D3D11 showed the physics demo's floor/boxes (no material - see meshsystem.h) wearing the
  BoomBox's own textures; D3D12 and Vulkan showed them a flat neutral color.** Root cause:
  `meshsystem.h` only calls `UseTexture()` when a `MeshRenderer` actually has a material, so an
  unmaterialed draw call arrives with no texture bindings set at all for that specific draw.
  `D3D11Renderer::Render()` bound `textureViews.size()` resources - which was 0, since the
  previous draw's cleanup had cleared the vector - and `PSSetShaderResources(slot, 0, ...)` is a
  D3D11 no-op, not an unbind, so whatever the *previous* draw call's textures were stayed bound.
  Confirmed directly: the floor/boxes rendered wearing the BoomBox's own textures, leaked forward
  from the draw call right before them in the same frame. D3D12Renderer/VulkanRenderer don't have
  this bug because both write a full, fresh set of bindings (falling back to a default texture)
  on every single draw regardless of whether `UseTexture()` was called that frame - D3D11 had no
  equivalent default-texture fallback at all. Fixed by giving `D3D11Renderer` the same
  `defaultTextureHandle`/`defaultTextureView` (1x1 white) D3D12Renderer/VulkanRenderer already
  had, always binding a fixed 6 slots (matching `bsdfPixel.hlsl`'s t0-t5) padded with that default
  rather than `textureViews`' current size, and resetting to the default (not clearing to empty)
  after each draw so the next one - even a materialless one - can't inherit stale bindings.
- **Vulkan showed the wrong face of some geometry** (concretely: the BoomBox model's handle
  rendered with a grille-patterned surface that D3D11/D3D12 both render smooth) **- the
  `VK_FRONT_FACE_CLOCKWISE` compensation from the upside-down fix above was itself wrong.**
  Worth recording precisely, since the reasoning that produced it was plausible and is a genuinely
  easy mistake to make twice: it's tempting to reason that a negative-height viewport must flip
  the winding the rasterizer perceives, and so `frontFace`/`cullMode` need to compensate - but
  that's only true relative to *Vulkan's own prior, independently-correct-for-Vulkan's-default-
  convention* setting. Here, Vulkan's `frontFace` was never independently verified correct for
  Vulkan's own default convention - it was copied from D3D11's value specifically *to match
  D3D11's rendering*, and the whole scene was upside down at the time anyway, which would have
  masked a culling problem underneath a more obvious one. Once the viewport flip makes Vulkan's
  NDC-to-screen mapping match D3D11's exactly, the two pipelines' winding computation becomes
  equivalent too - so the correct move was to leave `frontFace` matching D3D11's value unchanged,
  not flip it. Confirmed empirically (not just re-derived on paper a second time, given the first
  derivation's failure): reverting to `VK_FRONT_FACE_COUNTER_CLOCKWISE` fixed the artifact,
  screenshots compared directly against D3D11/D3D12 side by side.

**Where it lives:** `driver.d3d/d3d11renderer.h`/`.cpp` (default texture),
`driver.vulkan/vulkanrenderer.cpp` (`frontFace` correction).

---

## Phase 18 — Job/task system

**Depends on:** nothing structurally, but its real payoff starts once Phase 24 (culling), Phase
34 (particles), and Phase 44 (animation batch sampling/blending) exist to consume it.

**Why this wasn't on the original list:** Kay's list is entirely feature-facing; this is
infrastructure those features need to run well at any real scene scale. Right now there is no
general-purpose parallelism in the engine at all outside Jolt's own internal thread pool
(scoped strictly to physics) — culling hundreds of objects, sampling/blending dozens of
animated skeletons, or simulating many particle emitters all want to fan out across cores, and
none of that has anywhere to run yet.

**Scope + library choice:** **enkiTS** — zlib-licensed, ~2K lines, zero allocations during
scheduling (matters for avoiding game-loop jitter), designed specifically for game engines
(supports pinned/main-thread-only tasks, parallel-for task sets, task priorities), proven in
shipping titles. Chosen over Taskflow, which is a more powerful task-graph DSL but is overkill
for the fixed-shape-per-frame pipeline this engine actually needs (frame structure doesn't
change frame to frame the way an arbitrary DAG scheduler is built for).

**One real wrinkle to design around, not just implement:** EnTT's `registry` is not internally
synchronized — concurrent access to the same component pool from multiple job-system tasks needs
external synchronization or a data-partitioning scheme (e.g. each parallel task owns a disjoint
slice of entities, no shared mutable state). Decide this explicitly per-system as each one gets
parallelized rather than assuming EnTT "just handles it."

**Where it lives:** new `engine.jobs` module wrapping enkiTS, consumed by `engine.core`'s
systems as they're parallelized.

---

## Phase 19 — Deferred + forward hybrid rendering architecture

**Depends on:** nothing structurally, but this is the single highest-leverage phase in the
rendering track — almost every phase after it reads or writes data this phase defines.

**Why "hybrid" rather than picking one:** Kay's item 11 asks for "both deferred and forward" —
that's not an either/or in modern engines, it's the standard architecture: deferred can't
represent blended/transparent surfaces at all (a GBuffer stores one surface per pixel), so every
deferred renderer (Unreal, Unity HDRP, Frostbite) pairs a deferred opaque pass with a forward
pass for transparency.

**Scope:**
- GBuffer opaque pass: albedo, world-space normal, roughness/metallic, emissive, and **motion
  vectors** (cheap to add now while the pass is being built; Phase 22's TAA, Phase 35's motion
  blur, and Phase 36's upscalers all need it — retrofitting later means touching every material
  shader a second time).
- Deferred lighting pass consuming the GBuffer, replacing the current forward-only lighting in
  the existing BSDF pixel shader for opaque geometry.
- Forward pass for transparent/blended geometry, sharing the opaque pass's depth buffer and
  light lists.
- **A basic tonemap operator (e.g. ACES) belongs in this phase, not deferred to Phase 35** — the
  moment lighting is computed in HDR, *something* has to map it back to a displayable range to
  get a usable image at all. Phase 35 later extends this into full color grading (LUTs,
  artistic grading), but this phase needs at least the minimum viable operator.
- GBuffer channel budget needs headroom for Phase 27's clearcoat data — settle this once rather
  than resizing the format repeatedly across later phases.

**Where it lives:** `driver.vulkan/` first (per the D3D12-scaffold note above), pass structure
kept in `driver.graphics/renderer.h`'s shared interface for D3D11/D3D12 to follow later; new
HLSL in `res/ps/`/`res/vs/`, compiled through the existing DXC→SPIR-V pipeline.

---

## Phase 20 — Lighting: point, area, directional, spot (cone) lights

**Depends on:** Phase 19 (this phase's light model is what the deferred lighting pass and
forward transparent pass both consume).

**Why a separate phase from shadow mapping, and why it comes first:** Kay asked for this
explicitly, positioned next to shadow mapping. Checked the actual code — worth doing lighting
first and shadows second, not the other way round, because **shadow technique is a function of
light type**: cascaded shadow maps for directional, a single 2D map for spot/cone, a cube map
for point, and area lights need a fundamentally different (and much harder) approach. Nailing
down which light types exist, in their final scalable form, before building shadow maps against
them avoids redoing shadow infrastructure when a light type changes shape later.

**What's already there vs. what's genuinely new:** [light.h](../../engine.core/graphics/light.h)
already has a `Light` component with a `LightType` enum — **but it's `Spot`, `Directional`,
`Point` only. "Spot" and "cone" are the same concept** (a spot light *is* a cone of light) — so
three of Kay's four requested types already exist in some form. **Area lights are the real gap**
and the only genuinely new light *type* this phase needs to add.

**The bigger gap isn't light types, it's scale.** [shadercb.h](../../driver.graphics/shadercb.h)
hardcodes `MAX_LIGHTS 8` — a fixed-size array uploaded into the `worldConstant` cbuffer every
frame, every light unconditionally evaluated for every pixel in the forward BSDF shader
(`bsdfPixel.hlsl`), no culling of any kind. That's invisible with the current handful of demo
lights and becomes a hard ceiling the moment any real scene (Phase 15's editor, Phase 14's
importers) has more than 8 lights in it. This phase should replace the flat array with a
**clustered (or tiled) light culling scheme** — bin lights into screen-space tiles or
view-frustum clusters, evaluate only the lights relevant to each tile/cluster per pixel — which
pairs naturally with Phase 19's GBuffer (clustered/tiled deferred is the standard technique) and
extends to the forward transparent pass as clustered forward+, rather than needing two separate
lighting schemes for the deferred and forward halves of Phase 19's renderer.

**Scope:**
- Add `LightType::Area` to the existing enum, with shape variants (rectangle, disc, sphere —
  matching what Kay asked for as "area"). Real-time area lights need **Linearly Transformed
  Cosines (LTC)**, the standard technique (Heitz et al.) for analytically integrating a BRDF
  against an area light's shape — naive point-sampling (treating the area light as several point
  lights) doesn't converge to a correct result and looks visibly wrong at grazing angles/close
  range. This is a real, nontrivial addition, not just a new enum value.
- Replace `shadercb.h`'s fixed `GpuLight lights[MAX_LIGHTS]` array + linear per-pixel loop with
  the clustered/tiled culling scheme above; extend
  [lightsystem.h](../../engine.core/ecs/systems/lightsystem.h) accordingly.
- Directional/spot/point stay conceptually as they are today (already correctly modeled in
  `bsdfPixel.hlsl`'s `LightCalcSpot`/`LightCalcPoint`/inline-directional functions) — this phase
  carries that logic into the new deferred lighting pass and culling scheme rather than
  reinventing it.

**Where it lives:** `engine.core/graphics/light.h` (extend), `driver.graphics/shadercb.h`
(replace the fixed-array layout), `engine.core/ecs/systems/lightsystem.h`, new culling compute
pass in `driver.vulkan/`, `res/ps/`+`res/vs/` for the LTC area-light BRDF term.

---

## Phase 21 — Shadow mapping

**Depends on:** Phase 20 (shadow technique is per-light-type — cascades for directional, a 2D
map for spot/cone, a cube map for point, area lights need their own harder approach — so the
light types/system need to exist first; see Phase 20's "why here").

**Why this wasn't on the original list, and why it can't wait:** checked directly — there is no
shadow mapping anywhere in this engine today. This is arguably the most surprising gap in Kay's
original list, since it's normally considered part of "a renderer" rather than an optional
feature on top of one; almost every later lighting-quality phase (RTAO/raytraced shadows under
Phase 29, RTGI under Phase 30) is meaningless without a baseline shadow story to compare against
or fall back to.

**Scope:** cascaded shadow maps for the directional/sun light (multiple frustum-fitted depth
maps blended by distance from camera — the standard technique for large outdoor scenes), a
single shadow map per spot/cone light, and a cube shadow map per point light (or a capped count
of "shadow-casting" point lights, since 6 depth passes per point light gets expensive fast —
decide a budget/policy explicitly rather than letting every light cast shadows unconditionally).
**Area light shadows are explicitly out of scope here** — they're a meaningfully harder problem
(no single sensible "light position" to render a depth map from) and not implied by anything on
Kay's list; if Phase 20's area lights need shadows later, that's its own follow-up, not something
to assume this phase covers for free.

**Where it lives:** new pass in `driver.vulkan/` alongside the Phase 19 GBuffer/lighting passes;
new `res/ps/` shadow-depth-only shader variant.

---

## Phase 22 — Anti-aliasing (TAA / MSAA / FXAA)

**Depends on:** Phase 19 (TAA specifically wants its motion vectors and a jittered projection
matrix across frames).

**Why this wasn't on the original list:** the same gap-check that found no shadow mapping found
no anti-aliasing story either. This matters independently of whether/when Phase 36's DLSS/FSR/
XeSS upscaling lands — those subsume TAA's job when active, but a build without them still needs
*something* or edges alias badly. Worth having a baseline before the upscaling phase, not
instead of it.

**Scope:** TAA as the primary target (temporal accumulation using Phase 19's motion vectors,
history buffer, jittered projection matrix — this is also most of the groundwork Phase 36's
upscalers will reuse, so it's not wasted effort even once DLSS/FSR/XeSS exist), with MSAA and
FXAA as cheaper fallback options for lower-end hardware/simpler content.

---

## Phase 23 — Skybox / IBL

**Depends on:** technically nothing (simple enough to pull earlier if Kay wants a quick
standalone win) — sequenced here because it pairs naturally with Phase 19's lighting pass for
image-based-lighting ambient contribution.

**Scope:** cubemap background render, HDR environment map loading — the engine already has
cubemap support ([cubemap.h](../../engine.core/graphics/cubemap.h), `CreateCubemapSRV` exists on
all three renderer interfaces already — check how much is already usable before assuming this
needs building from scratch), optional IBL ambient term feeding into Phase 19's lighting pass.

**Where it lives:** `engine.core/graphics/` (extends existing cubemap support) +
`driver.vulkan/`.

---

## Phase 24 — GPU/CPU culling, LOD, GPU instancing

**Depends on:** Phase 19 (culls against/feeds the GBuffer pass), Phase 18 (CPU-side culling
wants to run across cores once scene object counts get large).

**Why this wasn't on the original list:** checked directly — there is no frustum culling,
occlusion culling, level-of-detail system, or GPU instancing anywhere in this engine; every
object is submitted individually and unconditionally every frame. That's invisible at demo-scene
scale (a handful of objects) and becomes the actual bottleneck the moment Phase 15's scene
editor lets Kay import a real scene with hundreds/thousands of objects — better to add this
before that becomes a painful surprise than after.

**Scope:** CPU frustum culling as the baseline (cheap, high value), occlusion culling as a
follow-up (Hi-Z/depth-pyramid-based, GPU-driven — meaningfully more complex, worth treating as
its own sub-milestone within this phase rather than assuming it ships alongside frustum culling
on day one), a simple distance-based LOD scheme (discrete LOD mesh swaps, not continuous/
nanite-style geometry — that's a much bigger undertaking and not implied by anything on Kay's
list), and GPU instancing for repeated meshes (a straightforward, high-value win for any scene
with foliage/props/repeated geometry).

---

## Phase 25 — Shader editor/creator

**Depends on:** Phase 19 (a shader graph has to compile into *something* — Phase 19's GBuffer/
forward pass shaders).

**Scope:** node-graph editor UI (ImGui + **imnodes** — a small, ImGui-native node-editor
library) emitting HLSL, compiled through the existing `dxc`-based pipeline, plus a *runtime*
"compile on save" path for iteration (the existing `res/CMakeLists.txt` pipeline is build-time
only). Node types at minimum: math ops, texture sample, the existing BSDF/PBR input slots, and
enough control flow (lerp/branch-via-select) to be useful without needing real branching in
generated code.

**Where it lives:** likely a new `engine.shadergraph` addition or folded into
`engine.editor/` — decide the module boundary when this phase starts.

---

## Phase 26 — Material editor/creator

**Depends on:** Phase 25 (materials are instances of shader graphs, or at minimum share the
compiled-shader pipeline).

**Scope:** PBR, Unlit, Custom (arbitrary Phase 25 shader graph), and Particle (with
billboarding — a billboard vertex-shader variant facing the active camera; note this is the
material-side half of what Phase 34's particle *system* needs, not the emitter/simulation half)
material types. Live preview (a sphere/plane rendered with the in-progress material) is the
highest-value UX feature here.

**Where it lives:** extends [bsdfmaterial.h](../../engine.core/graphics/bsdfmaterial.h) +
`engine.editor/`.

---

## Phase 27 — Displacement, subsurface scattering, parallax occlusion mapping, clear coat

**Depends on:** Phase 19 (GBuffer channel budget must already account for these), Phase 26
(exposed as material parameters, not hardcoded per-mesh).

**Scope, with a concrete recommendation per feature:**
- **POM:** screen-space/per-pixel raymarch in the pixel shader (samples a heightmap, no new
  pipeline stage) over true geometric displacement — this pipeline has no tessellation stage
  today, and adding one just for displacement is a much bigger lift than POM's pixel-shader-only
  trick for a lot of the same visual benefit at grazing angles.
- **True vertex/tessellated displacement:** stretch goal within this phase, gated on whether POM
  alone is good enough — would be the first use of a tessellation stage anywhere in this engine.
- **SSS:** pragmatic screen-space or precomputed diffusion-profile approximation, not a full
  BSSRDF/path-traced approach — consistent with this being a rasterized hybrid renderer.
- **Clear coat:** a second, fixed specular lobe on top of the base BSDF, following the standard
  Disney/Unreal clearcoat model — the most well-trodden of the four.

**Where it lives:** `res/ps/` (new HLSL variants of the BSDF pixel shader),
`engine.core/graphics/bsdfmaterial.h`.

---

## Phase 28 — Screen-space effects: SSAO / SSR / HBAO / decals

**Depends on:** Phase 19 (reads the GBuffer directly).

**Why decals folded in here:** projected decals (bullet holes, blood, grime) are a
GBuffer-writing, deferred-friendly technique in the same family as this phase's other passes —
grouping them avoids a separate thin phase for a closely related technique.

**Why SSAO/SSR/HBAO split from Phase 29's raytracing:** these need no new hardware capability
or API extension — pure compute/pixel-shader passes over the GBuffer, running identically on
D3D11/D3D12/Vulkan with zero capability-checking. Worth shipping before the raytracing-dependent
half of item 8: lower risk/effort, and a real engine-wide quality win on every backend
immediately.

**Scope:** SSAO baseline, HBAO (horizon-based — better quality than naive SSAO for similar cost,
worth doing instead of/alongside rather than as separate later work), SSR (reusing the
GBuffer's roughness for glossy, not just mirror, reflections), decals (a projected-box decal
pass writing into the same GBuffer channels the base materials do).

**Where it lives:** new post-GBuffer compute passes, `res/ps/` (or a new `res/cs/` compute
directory — none exists yet, checked) + a new pass in `driver.vulkan/`.

---

## Phase 29 — Hardware raytracing foundation (RTAO, real reflections, reflection probes)

**Depends on:** Phase 19 (GBuffer to combine RT results with), Phase 28 (SSR as the
capability-checked fallback path). D3D12 leg additionally depends on Phase 17.

**Scope:** acceleration structure build/update (BLAS per mesh, TLAS per frame) via
`VK_KHR_acceleration_structure`, ray dispatch/pipeline via `VK_KHR_ray_tracing_pipeline` on
Vulkan; DXR (`ID3D12Device5`, `DispatchRays`) once Phase 17 lands on D3D12. **D3D11 has no
raytracing capability at all — an API ceiling, not a driver gap** — needs an explicit
`Renderer::SupportsRaytracing()`-style capability query so gameplay/rendering code can request
"RTAO if available, SSAO otherwise" rather than assuming it's always there. Covers item 8's
remaining pieces: RTAO, true (not screen-space-limited) reflections, reflection probes for
spheres/planes/cubemaps.

**Where it lives:** `driver.vulkan/` (new acceleration-structure management alongside the
existing `HandlePool`-based resource pools), `driver.graphics/renderer.h` (new capability-query
+ raytracing-specific virtuals, mirroring how ImGui support was added in Phase 7).

**Toggle:** `DUPLEX_ENABLE_RAYTRACING` at the CMake level (see "Feature toggles" above); at
runtime this is really the `SupportsRaytracing()` capability query rather than a separate
on/off setting, since it's hardware-gated, not a taste preference.

---

## Phase 30 — RTGI

**Depends on:** Phase 29 directly (same acceleration structures, same capability-gating story).

**Toggle:** compiles in only when `DUPLEX_ENABLE_RAYTRACING` is on (it's built directly on
Phase 29); expose as an editor render-setting toggle once built, since unlike raw raytracing
support, "is RTGI worth its cost for this project" is a legitimate per-project quality choice,
not just a hardware yes/no.

**Scope:** research-heavy in its own right when actually reached — the two realistic concrete
choices are DDGI (probe-grid-based, cheaper, well-documented, good for mostly-static scenes) or
ReSTIR GI (higher quality, more complex, better for fully dynamic scenes). Defer the
DDGI-vs-ReSTIR decision to when this phase starts — the right choice depends on the kind of
content Kay is targeting by then, which isn't known now.

---

## Phase 31 — Lightmap baking

**Depends on:** Phase 29 (reuses its raytracing acceleration structures to compute baked
lighting), loosely Phase 30 (RTGI) and Phase 14 (importers, for the static level geometry being
baked).

**Not redundant with Phase 30's RTGI, even though both are about indirect lighting** — they
solve different problems and most shipped games that have either also have the other: baking has
zero runtime cost but is only correct for geometry and lighting that don't move, while RTGI
handles fully dynamic scenes at a real runtime cost. The common pattern is baked lighting for
static level geometry plus RTGI (or simpler dynamic-light-only shading) layered on top for
anything that moves — not a choice between the two.

**One existing hook worth using, found while scoping this:**
[transform.h](../../engine.core/ecs/components/transform.h) already has a `bool isStatic`
field on every `Transform` (added in Phase 11, unused since) — this is exactly the flag baking
needs to decide what's eligible to bake into a lightmap versus what stays dynamically lit.
Similarly, [light.h](../../engine.core/graphics/light.h)'s `Light` component already has an
`indirectMultiplier` field (mirrored in `GpuLight` as `indirectMul`) — presumably intended for
exactly this kind of "how much of this light's contribution comes from the bake vs. real-time"
balancing, sitting unused until this phase gives it a reason to exist.

**Scope:**
- **Lightmap UV generation**: [vertex.h](../../engine.shared/vertex.h) has only one UV channel
  (`texCoords`) today — needs a second, non-overlapping UV channel (a "UV1"/lightmap UV set)
  added to the vertex format, since imported meshes (Phase 14) won't come with one and a
  material's regular texture UVs are usually overlapping/mirrored in ways that don't work for a
  lightmap. **xatlas** (C++11, no external dependencies, MIT-style license, purpose-built for
  exactly this — originally written for the game *The Witness*) is the standard open-source
  choice for generating this second UV set from arbitrary mesh topology; used by several other
  engines/tools for the same job.
- **Baking compute**: reuse Phase 29's Vulkan acceleration structures (BLAS/TLAS) and ray
  dispatch to trace rays from each lightmap texel rather than each screen pixel, accumulating
  direct + indirect (bounced) lighting into a texture per static object/level chunk. Chosen over
  adding a separate CPU raytracer (e.g. Embree, the traditional choice for offline lightmap
  bakers like Unreal's older Lightmass) specifically to reuse Phase 29's investment instead of
  taking on a second raytracing dependency — consistent with how Phase 41/42 reuse Jolt's
  already-vendored `CharacterVirtual`/`Ragdoll` rather than building separately. Worth noting
  the industry has been moving the same direction (Unity's GPU Progressive Lightmapper largely
  superseding its older CPU/Embree-based one) — this isn't a contrarian choice, it's the current
  standard approach. If baking without RT-capable hardware ever becomes a real requirement,
  Embree is the documented fallback rather than a hard blocker on this phase.
- **Editor integration**: a "bake lighting" action in Phase 15's editor, triggered manually
  (not automatic — bakes can be slow), writing results out as texture assets through Phase 13's
  asset manager.

**Where it lives:** new `engine.core/graphics/lightmapbaker.h`/`.cpp` (or `engine.lightmap`
module if it grows large enough), `engine.shared/vertex.h` (extend with the UV1 channel),
editor integration in `engine.editor/`.

**Toggle:** the baking *tool* is naturally editor-only (`DUPLEX_ENABLE_LIGHTMAP_BAKING` gating
whether it's built into editor builds at all — a shipped game only needs to sample the already-
baked textures, not the baker itself); per-object "use the bake vs. stay dynamically lit" is
already covered by the existing `isStatic` flag, not a new toggle to invent.

---

## Phase 32 — Variable rate shading

**Depends on:** Phase 19 (applied per-draw/per-region against its passes). D3D12 leg depends on
Phase 17.

**Scope:** `VK_KHR_fragment_shading_rate` on Vulkan (per-draw, per-primitive, and
per-screen-region rates, combined by multiply in linear space); D3D12 Tier 1 (per-draw only) or
Tier 2 (adds per-primitive + a screen-space rate image, exact ≤16×16 tiles) once Phase 17
exists. **D3D11 has no VRS support** — same capability-query story as Phase 29. Content-adaptive
rate selection (lower rate in periphery/motion-blurred regions) is the actual payoff; exposing
the API without a policy for *when* to drop the rate isn't very useful alone.

**Toggle:** `DUPLEX_ENABLE_VRS` at the CMake level; runtime on/off (or per-quality-tier) is a
legitimate editor render-setting toggle once available, on top of the hardware capability query.

---

## Phase 33 — Order-independent transparency

**Depends on:** Phase 19 (specifically the forward transparent pass it defines).

**Note:** plain alpha blending (standard blend state, back-to-front sorted) is presumably
already how any existing transparent demo content renders — the real ask is *order-independent*
transparency, for cases sorting can't solve.

**Scope, with a concrete recommendation:** **Weighted Blended OIT** first — constant per-pixel
memory (unlike per-pixel linked lists' ~200MB-at-1080p-for-8-layers cost), single pass, no new
descriptor-heavy infrastructure. **Per-pixel linked lists (PPLL)** as an explicit stretch/
quality-tier upgrade later if Weighted Blended OIT's approximation artifacts turn out to matter
— PPLL gives pixel-exact results but needs unordered-access/storage-buffer-with-atomics
infrastructure this engine doesn't have yet.

---

## Phase 34 — Particle system / VFX

**Depends on:** Phase 26 (particle material type), Phase 18 (simulating many emitters wants to
fan out across cores or move to GPU compute).

**Why this wasn't on the original list:** item 5 mentions "Particle (with billboarding)" as one
material *type*, but a real particle *system* — emitters, spawn/lifetime/velocity simulation,
GPU-driven update for large particle counts — is a distinct, larger feature that a billboard
material alone doesn't cover. Worth calling out as its own phase rather than assuming it's
implied by the material-editor phase.

**Scope:** CPU emitters with a simple spawn/update/kill lifecycle as the baseline; GPU-simulated
particles (compute shader update, indirect-draw rendering) as the scale-up path once particle
counts matter enough to justify it. Emitter authoring (rate, lifetime, velocity/color-over-life
curves) is naturally an editor panel extension of Phase 15's editor.

---

## Phase 35 — Post-processing: bloom, motion blur, depth of field, color grading

**Depends on:** Phase 19 (its minimum-viable tonemap operator, extended here into full grading;
motion blur wants Phase 19's motion vectors).

**Scope:** bloom (bright-pass threshold + separable blur chain), depth of field (reuses the
existing depth buffer), motion blur (per-pixel motion-vector-driven, not reconstructed from
camera-only motion, which looks wrong for moving objects), and **color grading** — LUT-based
grading on top of Phase 19's baseline tonemap operator (this wasn't explicitly separated out in
Kay's original item 15, but "post-processing effects" implies more than the three named effects
once a real HDR pipeline exists to grade).

---

## Phase 36 — Upscaling: DLSS, FSR, XeSS

**Depends on:** Phase 19/22 (motion vectors + jittered projection matrix, the standard
TAA-adjacent input every modern upscaler wants).

**Research finding worth acting on:** **NVIDIA's Streamline SDK** is open-source and explicitly
designed as a cross-IHV plugin framework — a single integration point dispatching to DLSS, FSR,
and XeSS plugins interchangeably, rather than three parallel, differently-shaped integrations
(confirmed via NVIDIA's own developer docs/GitHub as of this research pass; re-confirm current
SDK version/terms when this phase starts, since upscaler SDKs move fast). Same kind of "one
well-chosen library instead of three bespoke integrations" call already made for physics (Jolt)
and ECS (EnTT).

**Scope note:** DLSS is NVIDIA-hardware-gated regardless of integration path; FSR is
vendor-agnostic; XeSS has a fast native (Intel XMX) path and a slower DP4a fallback for
non-Intel GPUs. Expect per-vendor capability checks even behind the unified Streamline entry
point.

**Toggle:** `DUPLEX_ENABLE_UPSCALING` gates whether Streamline/the vendor SDKs are linked at
all; which upscaler (or "none") is active is squarely an editor/project render-setting choice on
top of that, further narrowed by per-vendor hardware capability.

---

## Phase 37 — In-game GUI system: HTML/CSS UI + value injection

**Depends on:** Phase 19 for a composited scene texture to draw on top of. Positioned here —
after Phase 36, not right after Phase 19 — specifically for *within-frame ordering*, not because
it needs Phases 20–36's content: UI has to be drawn at native output resolution, after any
upscaler runs, not before it, or menu text/icons get soft/blurred by the upscale pass the same
way the 3D scene does. Every shipping DLSS/FSR/XeSS game follows this ordering (upscale the 3D
scene, then composite native-res UI on top, then present) — worth locking in now rather than
rediscovering it after Phase 36 ships and UI has to be retrofitted around it. Loosely benefits
from Phase 46 (scripting) once it exists, for UI event-handler logic beyond simple data binding
— not a hard dependency, see below.

**Not to be confused with Dear ImGui (Phase 7/15):** ImGui is the editor's dev-tool UI —
immediate-mode, fast to iterate, not meant to ship inside a game. This phase is specifically the
**player-facing UI** a shipped game needs: HUD, main menu, pause menu, dialogue, inventory,
settings screens. The two stay separate systems; nothing here replaces ImGui in the editor.

### Kay's question: is HTML/JS actually the right call, or is there something better?

Researched the real options rather than assuming HTML/JS is right just because some engines do
it that way. Four real candidates:

- **Coherent Gameface** — what most AAA studios actually license for exactly this (Cohtml, a
  from-scratch HTML5-standard-compliant engine, *not* Chromium/WebKit-based; Coherent's own
  numbers claim sub-1ms frame budget on PS4-class hardware; supports React/Preact/jQuery on top
  of standard HTML5/CSS). Ruled out for this project specifically: it's fully proprietary,
  commercial-only, no open-source tier at all — the same reason FMOD/Wwise were ruled out for
  Phase 39's audio stack. Worth naming since it's genuinely the industry benchmark, but it
  breaks this project's otherwise all-open-source dependency list (GLM, EnTT, Jolt, ImGui, SDL2,
  SPIRV-Cross) unless Kay specifically wants to pay for it.
- **CEF (Chromium Embedded Framework)** — open source (BSD-style, from the Chromium project),
  full modern HTML5/CSS3/JS, the most web-standards-complete option by far. Two real costs worth
  weighing against that completeness: (1) it runs the browser content in a **separate process**
  from the game, so C++⇄JS "value injection" has to go through Chromium's own IPC message-
  routing/`CefV8Value` bindings rather than a simple in-process call — meaningfully more
  integration complexity than the other options; (2) its offscreen-rendering (OSR) path is
  best-supported via **D3D11 shared textures on Windows** (GL-based OSR was deprecated upstream
  from CEF 104 onward), which is a real cross-platform-parity risk for an engine whose whole
  point is Windows+Linux via Vulkan — the Linux/Vulkan leg would more likely need to fall back to
  CPU bitmap readback-and-upload, meaningfully higher overhead than Windows' shared-texture path,
  and a real asymmetry between backends worth going in with eyes open about.
- **Ultralight** — a lightweight, pure-GPU HTML/CSS/JS renderer built specifically for embedding
  in games (JavaScriptCore-derived JS engine, genuinely full HTML5/CSS3, not a subset). Designed
  for exactly this integration pattern from the start: renders to a texture the engine uploads
  or, with a lower-level GPU driver path, writes to directly — no separate-process IPC story like
  CEF's. Licensing: free for non-commercial use and free for commercial use while under $100K/yr
  revenue, paid beyond that; a portion of its WebCore fork is LGPL/open on GitHub but the
  complete SDK is proprietary. If Kay specifically wants literal HTML/CSS/JS — React, existing
  web-dev skills/tooling, hiring web devs to build UI — **this is the recommended pick among the
  JS-literal options**, over CEF's heavier footprint/process-model cost and Gameface's fully
  closed licensing.
- **RmlUi** — genuinely open source (MIT), C++17, purpose-built for games rather than adapted
  from a browser (a fork of the old libRocket project). Uses an HTML/CSS-*inspired* markup/style
  dialect — a practical subset tuned for game UI, not a literal HTML5/CSS3 spec implementation —
  and its scripting story is Lua bindings plus its own first-party **data-binding (MVC) system**,
  not a full JS engine. Also ships decorators, sprite sheets, localization, and **built-in
  gamepad-friendly spatial navigation** (menu navigation via controller d-pad/stick out of the
  box) — a detail none of the HTML/JS options above provide as a first-party feature, and a real
  practical concern for anything meant to run on a gamepad.

**Recommendation: RmlUi**, over literal HTML/JS. Three reasons, not just one: it's the only
option that matches this project's established open-source-only dependency pattern without
carve-outs; its data-binding system is a first-party, purpose-built answer to Kay's "injecting
of values" ask (see below) rather than something to bolt onto a general-purpose JS engine; and
it reuses Phase 46's Lua investment for UI logic instead of adding a second, unrelated
scripting runtime (V8/JavaScriptCore) just for menus. **Flagging this clearly as a call to
confirm with Kay, not a unilateral decision** — the same treatment given to Phase 38's
environment rendering and Phase 47's networking — since Kay's own framing of the request
specifically named HTML/JS, and if literal web-standards UI (React, hiring web devs, reusing
existing HTML/CSS assets) genuinely matters, Ultralight is the better-fitting answer to *that*
specific goal than RmlUi is, and that's a legitimate thing to want.

### "Injecting of values into javascript" — what this actually looks like per option

This is the C++⇄UI data-binding mechanism, and it differs meaningfully by library:
- **RmlUi**: first-party **data binding (MVC)** system — register a data model in C++ (fields
  with direct bindings or getter/setter pairs), reference it from RML markup via
  `data-value="fieldName"`-style attributes, and call `model.DirtyVariable(...)`/`Update()` from
  C++ to push changes into the UI. Event handlers bind back to C++ (or, once Phase 46 lands, to
  Lua) the same way. This is the most direct, least-plumbing-required match for what Kay
  described — it's a designed-in feature, not something to build from scratch.
- **Ultralight**: lower-level but standard JS interop — the C++ API exposes the live
  `JSContextRef` (JavaScriptCore's C API), used to set global JS values/objects from C++ and
  expose C++-callable functions to JS via `JSObjectMakeFunctionWithCallback`-style bindings.
  Real and well-documented, but more manual wiring per value/function than RmlUi's structured
  model.
- **CEF**: `CefV8Value`/`CefV8Handler` bindings in the (separate) renderer process, bridged to
  the game's process via Chromium's IPC message-routing layer — the most architecturally heavy
  of the three, a direct consequence of CEF's multi-process design noted above.

**Scope boundary worth stating explicitly, given Phase 47 sits nearby in this document:** this
system is presentation-layer only. UI state must **not** be folded into Phase 47's rollback
state snapshots — the deterministic-lockstep simulation state and the UI layer are separate
concerns, and conflating them would both bloat rollback snapshots with irrelevant data and risk
non-deterministic UI timing (animations, hover state) leaking into simulation-affecting state.

**Where it lives:** new `engine.gui` module wrapping the chosen library, `Duplex::Gui`
namespace, rendered as a final compositing pass in `driver.vulkan/` (and later D3D11/D3D12)
after Phase 36's upscale pass, analogous to how Phase 7 already wired ImGui's draw data into
the renderer — the compositing mechanics (upload/blit a UI layer over the 3D frame) are similar
in shape even though the UI content itself comes from a completely different library.

**Toggle:** `DUPLEX_ENABLE_GUI` at the CMake level — a headless/dedicated-server-style build
shouldn't need to link RmlUi/Ultralight at all. Not meaningfully an editor runtime toggle in the
same sense as the rendering features above (a shipped game either has its UI or doesn't).

---

## Phase 38 — Environment rendering: terrain, water, volumetric fog (optional)

**Depends on:** Phase 19 (integrates with the same lighting/GBuffer pipeline).

**Why flagged optional, unlike everything else above:** these weren't on Kay's original list at
all, and — unlike shadow mapping/culling/logging, which are missing fundamentals any engine
needs — whether they're worth building is genuinely genre-dependent. A terrain system, water
rendering, and volumetric fog/atmosphere are a meaningful chunk of work each, and only pay off
if Kay is targeting outdoor/open-world content. **Confirm scope with Kay before starting any
part of this phase** rather than assuming all three are wanted — this is exactly the kind of
"ask before a big scope call" this project has consistently done for its other major decisions
(physics engine choice, determinism requirement, etc.).

**If pursued:** heightmap terrain with texture splatting is the standard baseline; water as a
animated-normal-map + screen-space-reflection surface is far cheaper than full FFT ocean
simulation and likely sufficient unless Kay specifically wants open-ocean content; volumetric
fog as froxel-based (camera-frustum-aligned volume texture) scattering, the current standard
technique, reusing Phase 21's shadow maps for light shafts/god rays.

**Toggle:** `DUPLEX_ENABLE_TERRAIN`/`DUPLEX_ENABLE_WATER`/`DUPLEX_ENABLE_VOLUMETRIC_FOG` as
separate CMake options, not one bundled flag — confirmed above that these three don't
necessarily get built together, so they shouldn't share a single on/off switch either.

---

## Phase 39 — Audio system: 3D spatial audio, reverb zones, occlusion

**Depends on:** nothing rendering-related — good candidate to run in parallel with any rendering
phase. Loosely benefits from Phase 11 (spatialized sources live on entities with a world
position) and Phase 40 (occlusion wants raycasts against the physics world).

**Research finding + recommendation:** two-layer stack, mirroring the "focused library, not a
monolith" pattern already used for physics/ECS:
- **miniaudio** as the base mixing/decoding/device-output layer — single-header, cross-platform
  (WASAPI/PulseAudio/ALSA and more under one API), open source, handles format decoding/device
  output/basic 3D positioning/doppler well on its own.
- **Steam Audio** (Valve, Apache-2.0, open source — confirmed via its own GitHub repo, not just
  "source available") layered on top for HRTF-based binaural spatialization, reverb zones, and
  **occlusion via raycasts against the already-integrated Jolt physics world from Phase 9** —
  no second, separate scene-query system needed just for audio.
- FMOD/Wwise (the usual commercial alternatives) considered and set aside — both proprietary,
  which doesn't fit this project's otherwise all-open-source dependency list (GLM, EnTT, Jolt,
  ImGui, SDL2, SPIRV-Cross all are).

**Where it lives:** new `engine.audio` module, `Duplex::Audio` namespace (`DUPLEX_NS_AUDIO`
added to `engine.shared/namespaces.h`, matching how `DUPLEX_NS_PHYSICS` was added in Phase 9).

---

## Phase 40 — Physics: soft bodies, constraints, collision events

**Depends on:** Phase 9 (already done) directly — extends the existing Jolt integration rather
than starting fresh.

**Scope:** Jolt already supports what this needs natively — mostly exposure/plumbing:
- **Soft bodies**: Jolt's native `SoftBodyMotionProperties` — expose as a new component
  alongside the existing `RigidBody` (rigidbody.h).
- **Constraints**: Jolt's constraint types (point, hinge, slider, cone, six-DOF, etc.) — expose
  through the ECS as component(s) referencing two `BodyHandle`s.
  [physicsworld.h](../../engine.core/physics/physicsworld.h)'s PIMPL structure already hides the
  Jolt-specific calls behind it.
- **Collision events**: Jolt's `ContactListener` — bridge contact-begin/end callbacks into an
  EnTT signal or a queue drained once per frame in `PhysicsSystem::Update`.

**Where it lives:** `engine.core/physics/` (extends existing files), `engine.core/ecs/
components/` (new components alongside `rigidbody.h`).

---

## Phase 41 — Character controller (+ optional vehicle physics)

**Depends on:** Phase 40 (extends the same Jolt integration).

**Why this wasn't on the original list, and why it's cheap:** checked the vendored Jolt source
directly — `Jolt/Physics/Character/CharacterVirtual.h` is already in the repo (pulled in by
Phase 9's `engine.jolt` submodule), a full kinematic character controller (capsule sweep-based
movement, step/slope handling, standard-body interaction) that nothing currently wraps or
exposes. A kinematic character controller is close to a hard requirement for any game with a
player character that isn't purely ragdoll/rigid-body-driven, and most of the hard part is
already vendored.

**Scope:** wrap `JPH::CharacterVirtual` behind `PhysicsWorld`'s existing PIMPL pattern, expose
move/jump/ground-check through a `CharacterController` component + system.

**Optional, flag with Kay:** Jolt also vendors wheeled/tracked vehicle constraints
(`VehicleConstraint`, `WheeledVehicleController`, `TrackedVehicleController` — all already in
the repo). Not scoped in detail here since it's clearly genre-dependent (only worth it if Kay's
games actually have vehicles) — mentioned so it's not missed if that need comes up, since like
the character controller, most of the hard part is already vendored and unused.

---

## Phase 42 — Ragdoll physics

**Depends on:** Phase 40 (constraints), Phase 44 (skeleton to drive).

**Why this wasn't on the original list, and why it's cheap:** same finding as Phase 41 — Jolt
already vendors `Jolt/Physics/Ragdoll/Ragdoll.h`, native ragdoll support, unused. This phase is
primarily about blending physics-driven ragdoll bone poses with Phase 44's animated poses (e.g.
partial ragdoll on hit reactions, full ragdoll on death), not building ragdoll simulation itself
from scratch.

---

## Phase 43 — Navigation & pathfinding

**Depends on:** Phase 14 (level geometry to bake a navmesh from), ideally Phase 18 (async
navmesh baking).

**Why this wasn't on the original list:** any game with AI-controlled movement over level
geometry needs pathfinding, and nothing on Kay's original list covers it — a gap in the same
category as shadow mapping (a missing fundamental, not a stretch feature), just less visually
obvious than a lighting gap.

**Research finding:** **Recast/Detour** (`recastnavigation`) — confirmed still the industry
standard as of this research pass, used by Unreal, Unity, Godot, and O3DE, C++98-compatible
(trivially portable), actively maintained. No reason to look further than the de facto standard
here.

**Scope:** navmesh generation from imported (Phase 14) level geometry, runtime path queries,
basic dynamic obstacle avoidance (Detour's crowd/obstacle features).

---

## Phase 44 — Animation: skeletal animation, blend shapes, inverse kinematics

**Depends on:** Phase 11 (skeletons are joint hierarchies — a direct application of Phase 11's
parenting work, specialized to bones), Phase 14 (glTF is the primary source of skinned meshes
and animation clips).

**Research finding + recommendation:** **ozz-animation** — confirmed C++17 with no OS-specific
code, tested cross-platform (Linux/macOS/Windows/WASM, x86/x86-64/ARM), matching this project's
own cross-platform/portability bar from Phases 3 and 9. Provides playback/sampling/blending
(including partial and additive blending), joint attachment, and — covering item 18's IK ask
directly — built-in two-bone IK, look-at IK, and foot IK jobs. Its offline toolchain converts
from glTF into ozz's runtime-optimized structures, lining up directly with Phase 14's importer.

**One gap, needs a custom solve:** ozz-animation does **not** provide blend shapes/morph
targets (confirmed — explicitly not planned as an official feature, by design). Needs a small
amount of custom work alongside ozz: per-vertex delta storage per morph target, GPU-side
blending (compute pass or vertex-shader weighted sum over active targets), driven by weights
the animation system feeds in.

**Where it lives:** new `engine.anim` module wrapping ozz, `Duplex::Animation` namespace
(`DUPLEX_NS_ANIM` in `engine.shared/namespaces.h`), new ECS components (`Skeleton`,
`AnimationPlayer`, `MorphTargetWeights`) in `engine.core/ecs/components/`.

---

## Phase 45 — Animation state machines / blend trees

**Depends on:** Phase 44 directly.

**Why this wasn't on the original list, and why it's split out from Phase 44 rather than folded
in:** ozz-animation is a sampling/blending *library*, not an authoring layer — it has no concept
of states, transitions, or blend trees, and nothing fills that gap by default. Almost every
game with more than one animation per character wants this (idle→walk→run blending, transition
rules, layered upper/lower body states), so it's a near-certain follow-on to Phase 44 rather
than a speculative extra. Split into its own phase because it's substantial enough to warrant
its own scope, and has its own editor-tooling angle (a visual state-machine graph, naturally
hosted in Phase 15's editor once this exists) that Phase 44 itself doesn't need.

**Scope:** a state graph (states = animation clips or blend trees, transitions with conditions
sourced from gameplay/script state), blend-tree nodes (1D/2D blend spaces for e.g.
speed-driven locomotion blending) evaluated on top of ozz's blending primitives.

---

## Phase 46 — Scripting system

**Depends on:** Phase 11 (scripts reference entities), loosely Phase 12 (script-owned state may
need to serialize into saves).

**Why this wasn't on the original list, and why it matters:** checked — there is currently no
way to write gameplay logic without recompiling C++. That's a significant gap for an engine
whose editor (Phase 15) is meant to let Kay (or anyone else) build actual content — without a
scripting layer, every piece of gameplay behavior needs an engine rebuild.

**Research finding + recommendation:** **Lua, embedded via sol3** (sol2 v3) — header-only, the
most feature-complete and fastest C++⇄Lua binding available, ergonomic (close to standard-
library-style C++ usage, minimal boilerplate). Real tradeoff worth flagging up front: sol3
leans on heavy C++ template metaprogramming, which can mean slow compile times and dense
template error messages — worth confirming that's an acceptable tradeoff with Kay before
committing, given this project's build is already fairly involved (DXC, multiple vendored
submodules). Considered against AngelScript (closer to C++ syntax, but comparatively short on
modern language features) and Wren (small/fast, but a much smaller ecosystem/community than
Lua) — Lua's ubiquity in shipped games and tooling ecosystem tips the choice in its favor absent
a specific reason to prefer one of the alternatives.

**Scope:** entity/component bindings exposed to Lua (transform access, component queries),
a script component that binds a Lua script to an entity with lifecycle hooks (init/update/
destroy), hot-reload of scripts (natural pairing with Phase 13's asset hot-reload).

**Where it lives:** new `engine.script` module, `Duplex::Script` namespace.

**Toggle:** `DUPLEX_ENABLE_SCRIPTING` at the CMake level (sol3 + Lua only linked when on).

---

## Phase 47 — Networking: deterministic lockstep / rollback multiplayer

**Depends on:** Phase 9 (already done — `CROSS_PLATFORM_DETERMINISTIC` Jolt physics, verified
thread-count-independent), Phase 3 (already done — fixed-point math for genuine cross-platform
determinism), Phase 12 (state (de)serialization, needed for rollback's save/restore-state step).

**Why this wasn't on Kay's original list, and why it's flagged rather than just added:** this is
the most speculative addition in this whole document, and the only one being explicitly flagged
as needing Kay's confirmation before any work starts — multiplayer is a large, standing
architectural commitment (input handling, netcode, UI for lobbies/matchmaking are all downstream
of this decision), not something to assume is wanted just because the engine happens to be
well-suited for it.

That said, the fit is real and worth stating plainly: Phases 3 and 9 did not invest in
bit-exact cross-platform determinism for its own sake — that kind of investment is normally made
specifically to enable **deterministic lockstep** networking (every client simulates identical
inputs and reaches bit-identical state, so only compact input packets need to cross the
network, not full state) and its modern refinement, **rollback netcode** (GGPO-style: predict
opponent inputs, simulate ahead, roll back and resimulate on misprediction, so this style of
netcode can hide latency without adding input delay). GGPO itself has been open source (MIT)
since 2019 and the ecosystem around the technique remains active in 2025–2026 (rollback plugins
for Unreal, Godot, GameMaker are all live/maintained as of this research pass) — this isn't a
dated technique.

**Scope, if confirmed wanted:** fixed-timestep input capture and serialization (already
naturally aligned with Phase 9's fixed 60Hz physics accumulator), state snapshot/restore built
on Phase 12's serialization, a rollback buffer (N frames of input history + periodic state
snapshots), and a transport layer (UDP, with GGPO or a GGPO-inspired implementation as the
starting point rather than building rollback's input-prediction/resimulation logic from
scratch).

**Toggle:** `DUPLEX_ENABLE_NETWORKING` at the CMake level — this is the clearest case in the
whole document for a build-time toggle mattering, since a single-player-only build shouldn't
carry rollback/transport code or its dependencies at all.

---

## Phase 48 — Export / publish / bundle a game

**Depends on:** loosely everything above — deliberately last because a packaging/export format
wants the feature set to have mostly stopped changing shape.

**Scope:** asset cooking (bake/compress textures, package the already-build-time-compiled
shader artifacts from `res/CMakeLists.txt`'s DXC→SPIR-V step rather than shipping raw HLSL), a
manifest/pack file format bundling cooked assets, and a stripped runtime launcher target
distinct from the editor executable (the editor links ImGui/editor-only code the shipped game
doesn't need).

**Explicitly flagged for Kay to weigh in on:** the full scope above is naturally late-stage, but
a much smaller MVP ("zip the already-built binary + `bin/data/` folder together") is nearly
trivial and could be pulled forward if Kay wants *something* shippable sooner, well before the
rest of this roadmap is done. Worth asking explicitly when this phase is reached (or sooner)
rather than assuming the full scope is required from day one.

---

## Phase 49 — Sample project: a CMake-generated showcase scene from open-source assets

**Depends on:** in the sense of "fully exercises," practically everything above. In the sense of
"can start," only Phase 14 (importers) and Phase 15 (editor) are hard prerequisites — see "why
here" for why this should not actually be built only once, at the end.

**Why last in this document but not last in practice:** Kay asked for this as a last phase, and
it's placed last here because that's genuinely where it belongs in a *dependency* sense — it's
the one thing in this roadmap whose entire point is to exercise everything else. But a sample
scene that only gets built after Phase 48 has never actually run against the engine while any of
Phases 20–48 were being built, which throws away most of its value as a running integration
check (the kind of thing that would have caught, for example, a GBuffer channel budget mistake
in Phase 19 well before Phase 27's clearcoat data needed the headroom). **Recommend starting a
minimal version of this as soon as Phase 14/15 exist, and extending it by a few entities/assets
each time a later phase lands**, rather than treating "Phase 49" as a single big-bang effort
saved for the very end. The phase is numbered last because that's when it's *complete*, not when
work on it should *start*.

**What "CMake can generate it" means concretely:** not asking CMake to invent a scene — CMake's
job is fetching the binary assets (models/textures/HDRIs/audio) that a small, hand-authored,
git-tracked scene file (using Phase 12's serialization format) references. The scene composition
itself is authored content, checked into `samples/`; the *assets* it points at are fetched on
demand rather than vendored into the repo, the same way `engine.jolt`/`engine.entt`/etc. are
submodules rather than copied source — except here as a fetch step, not a submodule, since these
are content files being selectively pulled from large asset repositories, not a whole library.
Gated behind a new `DUPLEX_BUILD_SAMPLE_PROJECT` CMake option (off by default — it triggers
network fetches and isn't needed for core engine development), added via
`add_subdirectory("samples")` guarded by that option, mirroring how the top-level
`CMakeLists.txt` already conditionally adds `driver.d3d`/`driver.d3d12` behind `if(WIN32)` —
the same conditional-subdirectory pattern already established in this exact file, not a new one.

**Fetch mechanism:** `file(DOWNLOAD ... EXPECTED_HASH ...)` pointed at specific files from
specific assets, not a blanket `FetchContent_Declare` of an entire asset repository — the asset
sources below (especially glTF-Sample-Assets) are large collections where only a handful of
individual models are actually wanted, and pinning `EXPECTED_HASH` per file catches silent
corruption/tampering for free. For an asset whose files are too numerous to enumerate by hand,
a shallow `git clone --filter=blob:none --sparse` + `git sparse-checkout set <model-folder>` is
the fallback, pulling only that one model's folder out of the larger repo rather than the whole
thing.

**Asset sources, and what each one demonstrates — picked deliberately, not just "some models":**
- **[KhronosGroup/glTF-Sample-Assets](https://github.com/KhronosGroup/glTF-Sample-Assets)** —
  the standard glTF conformance/feature-coverage asset set, which lines up directly with Phase
  14 making glTF the primary import format. Concrete picks: a PBR metallic/roughness reference
  grid (e.g. `MetalRoughSpheres`) for Phase 26's material coverage; `DamagedHelmet` as the "hero"
  asset exercising every texture slot `BsdfMaterial` already has (base color, metallic-roughness,
  normal, occlusion, emissive) in one object; a large multi-room scene (Intel's Sponza) as the
  stress test for Phase 19/20's lighting and culling, Phase 21's cascaded shadows, Phase 28's
  screen-space AO, and Phase 30/31's GI (its arches and curtained rooms are the standard
  bounced-light test case for exactly this reason); an animated skinned mesh with multiple named
  clips (e.g. `Fox`, which ships Survey/Walk/Run as separate clips) for Phase 44's skeletal
  animation and Phase 45's blend-tree/state-machine work; a morph-target asset (e.g.
  `AnimatedMorphCube`/`MorphStressTest`) for Phase 44's custom blend-shape gap-fill specifically.
- **[Poly Haven](https://polyhaven.com/)** — genuinely CC0 (verified: public-domain-equivalent,
  no attribution required, confirmed via their own license page), used for an HDRI environment
  map for Phase 23's skybox/IBL. The cleanest of the three sources licensing-wise, worth
  preferring here whenever it has something that fits.
- **[Kenney.nl](https://kenney.nl/)** — also genuinely CC0 across its entire catalog (verified)
  — its UI Audio and Digital Audio packs cover Phase 39's audio system (SFX, good material for
  testing reverb zones/occlusion), and its UI/font packs give Phase 37's GUI system real content
  to style instead of programmer-art placeholders.
- **Hand-authored/procedural geometry — deliberately NOT fetched from anywhere.** A simple floor
  + ramps + a few obstacles (the same `Mesh::GenerateFlatCube()`-based approach Phase 9's demo
  scene already uses) is enough to exercise Phase 40's soft bodies/constraints, Phase 41's
  character controller, Phase 42's ragdoll, and Phase 43's navmesh baking — there's no reason to
  go fetch an external asset (and take on its licensing) for content the engine can already
  generate itself for free.

**Licensing due diligence is a real requirement here, not boilerplate caution — this exact
repository has a documented incident to prove it:** glTF-Sample-Assets'
[issue #172](https://github.com/KhronosGroup/glTF-Sample-Assets/issues/172) records that a
version of Sponza circulating in that repo actually violated Crytek's original license terms,
and the fix was to replace it with Intel's remake (CC-BY 4.0, attribution required). Individual
models in that repo carry individual licenses — CesiumMan, for instance, is CC-BY 4.0, not CC0
— confirmed by checking, not assumed. **Scope implication:** the fetch step must also pull each
model's own `README.md`/license file alongside its geometry, and this phase ships a consolidated
`samples/CREDITS.md` crediting every CC-BY asset by name — Poly Haven and Kenney content needs
no entry there (genuinely CC0), but anything sourced from glTF-Sample-Assets needs its license
checked per-model before use, not assumed safe because the collection as a whole sounds "open."

**Also doubles as a real exercise of two other phases, not just a passive showcase:** running
across D3D11/D3D12/Vulkan and varied hardware makes this the first place Phase 29/32's
`Renderer::SupportsRaytracing()`/VRS capability-query fallback paths (raytraced reflections vs.
SSR, RTGI vs. baked lighting, VRS on vs. off) actually get exercised on real content instead of
staying theoretical until someone happens to test on the right GPU. It's also the natural
acceptance test for Phase 48's export/bundle tooling — "can this sample project actually be
cooked and packaged into a shippable build" is a concrete, checkable question that toy content
wouldn't stress in the same way.

**Where it lives:** new top-level `samples/` directory (sibling to `engine.core`,
`driver.vulkan`, etc.) with its own `CMakeLists.txt` (the fetch logic + `add_subdirectory`
wiring), a small hand-authored scene file using Phase 12's serialization format, and
`samples/CREDITS.md`.

---

## Notes for whichever Claude session picks this up

- This file is the plan, not a log — update phase sections as decisions get made (e.g. "USD:
  decided against, glTF-via-Blender-export covers it," "networking: confirmed wanted, going with
  rollback not plain lockstep," "sol3 compile times were fine/were a problem, did X about it"),
  the way earlier phases' scoping decisions were tracked in this project's memory. Don't let
  this drift out of sync with what actually gets built.
- Every phase above inherits the standing project conventions already established in Phases
  0–9: no git commits without being explicitly asked, terse `[+]`/`[*]` commit message prefixes
  matching existing history, verify technical claims directly (compile/run/screenshot/diff)
  rather than trusting memory or docs, do a full call-site inventory before touching existing
  code, and ask before big scope calls rather than assuming the minimal option.
- Phases marked optional/flagged above (37: in-game GUI — HTML/JS vs. RmlUi, 38: environment
  rendering, 47: networking) are the ones most likely to be wrong about what Kay actually wants
  — confirm before investing real time, not just before committing code.
- Phase 49 (sample project) is numbered last but scoped to start as soon as Phase 14/15 exist —
  don't wait for Phase 48 to touch it. If you're implementing Phase 20 or later and the sample
  project already exists, extend it with that phase's content rather than leaving it stale.
