# Author guide — ABI 1

## Ownership model

HellForge already allocates private damage and projectile IDs. Explosives Library allocates private explosion IDs and maintains one shared engine-table extension. A consumer owns its gameplay choices and content; the provider owns the explosion table and registered explosion storage.

Use globally unique names such as `yourname/yourmod/heavy_blast`. Every cooperating consumer discovers the same provider. No static linking to the provider DLL or fixed DLL filename is required; the header locates the exported `explosives_library_get_api` function among loaded modules.

## Initialization sequence

1. In `hellforge_mod_init`, register damage with `HfApi.add_damage`. Supply a native damage base (1–649), a stable name, `size`, and a scale. Register your projectile with `add_projectile`; assign its direct-hit damage separately.
2. Subscribe to `HF_EVENT_PROJECTILES_READY`. Do not access added damage/projectile rows until that event.
3. Use `on_game_thread` to wait for the provider. `ElFind()` returning a pointer only means the provider ABI is available, not that its engine registry is ready.
4. Access your private damage row through `damage_row(returned_id)`. Verify its ID before editing it. Set standard/durable damage, armor penetration, demolition, stagger, push, or supported existing status fields as needed. Never substitute a shared native damage row.
5. Fill an `ElExplosion` and call `register_explosion` on the game thread. A zero result can mean the provider or asset package is not ready; retry with a modest delay. Inspect `status()` and provider logs if it persists.
6. Only after receiving a nonzero explosion ID, assign it to a private projectile's `impact_explosion`. Decide separately whether `expiry_explosion` should use it or be zero. Connect that projectile to your own weapon, grenade, sentry, or stratagem delivery records.
7. Keep every returned ID in session memory. Obtain IDs again on the next launch.

The [standalone example](examples/consumer.c.txt) registers a private Crossbow bolt and blast and routes that bolt into the existing Crossbow. Copy it as `src/example.c` in a **separate** test mod using SDK 15. It is shipped as `.txt` so HellForge does not accidentally compile it as part of the provider. It is a gameplay overwrite for testing, not something to add alongside other Crossbow edits.

A minimal manifest for that separate example mod is:

```json
{
  "format": 1,
  "id": "example.explosives-consumer",
  "name": "Explosives Library Example",
  "version": "0.1.0",
  "description": "Test-only Crossbow overwrite requiring Explosives Library.",
  "gameplay": true,
  "code": { "dll": "example_consumer.dll", "sdk": 15 }
}
```

Your mod folder contains this `hellforge-mod.json`, `src/example.c`, and `src/explosives_library.h`. Import it using your normal HellForge source-mod workflow. Change the example namespace to your own before publishing. The example is compile-checked; its exact 300-damage/5 m configuration has not been tested in game. The separate OPS debug consumer has runtime evidence.

## Three separate layers

| Layer | Defines | How to customize |
| --- | --- | --- |
| Projectile | Flight, direct hit, impact/expiry links, projectile appearance | HellForge private projectile registration and its private row |
| Explosion | Blast radii, explosion effect/audio and additional opaque metadata, damage link | This registry clones a donor; ABI 1 exposes damage/radii/shrapnel overrides |
| Damage | Standard/durable damage, AP, demolition, stagger, push, existing status fields | HellForge private damage registration and its private row |

Changing a projectile's appearance does not automatically change its explosion or the weapon's firing sounds. A visible blast does not establish that radial damage happened. A native projectile fuse can also detonate it in flight: use an appropriate delivery carrier rather than copying a short-lived plasma projectile's flight behavior into artillery.

## Damage and explosion sharing

Two private explosions can intentionally reference the same private damage row. Editing that row then affects both. Allocate separate damage rows if you want separate tuning. Similarly, do not give two different definitions the same explosion key.

An explosion is copied once at registration. Later edits to the donor's radius or damage link do not update the private row. However, edits to any damage row your explosion actually references still matter. The registry requires a damage ID that HellForge exposes through `damage_row`, normally a private ID returned by `add_damage`; native damage IDs such as 61 are not accepted by this API.

Package and visual metadata are not deep-cloned assets. Supply the correct native package hash for your explosion's effects or hold it yourself. A nonzero `package` makes the library request and retain it through the library mod; registration returns zero until it is resident. A zero package means the consumer is responsible for asset residency.

## Multiple consumers and failure handling

Registration is game-thread-only. Definitions are immutable, with no unregister or update operation. Register once and cache the ID. Repeating the same key and effective settings returns the same ID without consuming another slot. A conflicting definition with that key returns zero. Names must contain `/` and be at most 127 bytes long.

Load order must not be used as a readiness signal. Consumers should continue waiting without connecting invalid IDs, and log an understandable message if the provider is absent or disabled. The example leaves the stock Crossbow untouched until registration succeeds.

The provider reserves 64 slots in one engine patch. It rejects an already-extended table from an external patch rather than trying to chain extensions. Disable older lab builds and duplicate providers. Do not bundle a hidden second provider into a consumer.

This is a runtime C ABI, not a HellForge dependency declaration or automatic installer. State the library requirement on your mod page. Keep the public header's ABI layout intact. On a future incompatible API change, consumers must explicitly adopt the new ABI.

Do not unload the provider during play: engine pointers refer to its rows and allocated table. Use a full game restart after changing the mod set. Gameplay and asset ownership requirements remain the consumer's responsibility.

## 0.2.0 editor integration

Register custom projectile/damage records during initialization so they are present when the built-in editor reads its catalogue. Use stable namespaced names; renaming them can orphan saved edits. The editor integration does not add automatic readiness retries to consumers. Existing ABI 1 consumers need no rebuild. Runtime editor overrides are separate from immutable registration definitions. See [EDITOR.md](EDITOR.md).
