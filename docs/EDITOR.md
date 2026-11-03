# Built-in HellForge Editor integration

EL 0.2.0 works with the normal launcher-generated editor. The release does not distribute an editor replacement and removes the lab's binary-specific numeric-validation/staging probe.

## Discovery and identity

At catalogue-read time, EL enumerates HellForge custom content. It copies reviewed native field templates and redirects those fields to actual custom projectile/damage IDs. Injected entries retain stable registered mod/name/field keys. EL impact views resolve their projectile's current explosion and its damage record when read, handling EL registrations that occur after catalogue loading.

Display names retain the namespace/source and omit the internal `/library/` segment. Named payload references correspond to these views. They are UI aliases; EL translates selections to actual game IDs. Shared payload views are not independent explosions.

## Apply, reset, verify

Confirm typed input with Enter/Tab and APPLY/F9. RESET also needs APPLY. Redeploy active equipment/fire a new shot when comparing results. Test values one field at a time, then check reopening and restarting separately.

An example consistent radius change is 6/8/10 -> 5/7/9. An outer radius of 20 with shockwave 10 is invalid. Damage and penetration are separate from radius; AoE damage tests require an off-target hit at a controlled distance.

The library captures a custom field's baseline immediately before its first editor write. Later reads use that captured baseline; reset requests use the normal backend and recover the captured value if readback shows the reset did not restore it. EL radii reset to immutable registration-time defaults. Saved sandbox changes, including removals, are replayed into read-only stats snapshots when the stock editor reloads them. No installed stats file is rewritten by this integration.

## Logs

Read the latest session's `game/mod-hade.explosives-library.log` and `mod-hellforge.editor.log`.

- `EL INPLACE ARMED`: hooks installed; this does not prove interception.
- `EL INPLACE CATALOG: injected`: custom entries supplied to the stock editor.
- `EL INPLACE READ` / `EL INPLACE DAMAGE READ`: runtime record resolution.
- `EL INPLACE VERIFY`: accepted flag, before/after/base/requested values and readback result.
- `EL INPLACE WRITE` / `DAMAGE WRITE`: write routing or refusal.
- `EL EDIT APPLIED` / `REFUSED`: queued radius request actually applied or failed validation.
- `EL INPLACE SAVED OVERLAY`: sandbox journal replay on editor reload.

No injected entries: check provider enablement, duplicates, order and startup. A later registration cannot be added to an already-read catalogue by this release.

A value disappears **before APPLY**: no library write has occurred; report the selected page, exact field/value, keyboard confirmation used and whether other consumers reproduce it. This is reported with OPS Debug and remains unresolved. Do not assume radius validation explains a pre-APPLY disappearance.

A value reverts **after APPLY**: inspect backend refusals, radius ordering and consumer profile updates. A successful setter/readback does not alone prove gameplay damage.

## Reporting

Include library/editor/game versions, enabled consumer mods, relevant session logs, exact input values, whether reset/reopen/restart was involved, and whether numeric IDs or registered names changed. Multiplayer and ID-reassignment persistence are unverified. Do not report private account tokens or complete account snapshots.
