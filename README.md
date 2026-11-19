# Orbiter Economy Module (OEM)

An offline economic and career layer for the Orbiter Space Flight Simulator. Orbiter remains responsible for simulation physics, vessels, orbital mechanics, scenarios, and docking. This project adds markets, contracts, cargo manifests, company progression, and persistence.

## Project status

**Phase 0 — repository and architecture foundation.** The first implementation target is a small, testable economy core that is independent of Orbiter SDK headers. The Orbiter integration layer will be enabled only after the installed Orbiter build and matching SDK are confirmed.

## Development

See [`Docs/ARCHITECTURE.md`](Docs/ARCHITECTURE.md), [`Docs/ROADMAP.md`](Docs/ROADMAP.md), and [`Docs/BUILDING.md`](Docs/BUILDING.md).

The `development` branch is the active integration branch. Changes are kept incremental; a build is not considered verified until it has been compiled and exercised in the target Orbiter installation.

## Principles

- Never fake a docking, location, cargo transfer, or contract completion.
- Keep Orbiter simulation time authoritative.
- Persist economic transactions safely and make payouts idempotent.
- Keep the economy usable offline.
- Preserve compatibility with stock and third-party vessels through explicit adapters.
- Separate the portable economy core from Orbiter SDK integration.

## License

License selection is pending. Do not assume this project may incorporate code from third-party Orbiter add-ons.