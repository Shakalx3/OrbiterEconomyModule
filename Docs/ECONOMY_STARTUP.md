# Starting a clean OEM economy

The native bridge now uses one file, `OEMData/economy.journal`, for all future cargo exchanges and market trades. The older `cargo_exchanges` and `cargo_manifests` test journals are ignored and are not imported.

## Opening state

The following files are read beside the module, under `OEMData`:

- `economic_stations.registry`: player-managed orbital station identities. Register or remove the focused vessel in-game using the OEM Economic Admin MFD (key A). OEM creates and maintains this file; do not edit it manually.
- `cargo_services.cfg`: trusted cargo-service permissions, maintained in-game from the same Admin MFD. A cargo-service vessel is not automatically an orbital station; these are independent roles.
- `cargo_stock_<HEX location ID>.cfg`: explicit opening stock for each registered station vessel or planetary surface base. For a planetary base, OEM automatically creates this file on its first confirmed landing, using the exact Orbiter base name; the new file contains an empty inventory. For a registered station, the file is created when its inventory is first initialized if it does not already exist. Existing files are never replaced automatically.
- `accounts.cfg`: optional until market trading is enabled. If absent, the company ledger starts with no accounts and market trades cannot be executed.
- `cargo_catalog.cfg`: optional until market trading is enabled. If absent, there are no defined tradeable cargo types.
- `objective_mission_boards.dat`: generated, versioned daily offers for independent visit and docking missions. OEM creates and atomically updates it; do not edit it manually. Accepted mission state and payouts are stored in `economy.journal`, not in this board file.

The native Location Market MFD (key M) uses the company account ID `company`. Market trades are enabled only when `accounts.cfg` defines that account and `cargo_catalog.cfg` defines at least one item. Trade buttons buy or sell one unit of the first catalog item for which the station or vessel has available quantity.

No starting money, cargo definitions, prices, or stock quantities are generated automatically. Automatic location registration creates only an empty stock file; it does not add cargo.

## Example account configuration

This is an example only. Choose the starting balance deliberately before enabling market trading.

```
OEM_ACCOUNTS|1
A|636F6D70616E79|10000
```

The hex account ID `636F6D70616E79` is `company`. The example opening balance is 10,000 credits.

## Example cargo catalog

This is an example definition, not a built-in game item or default price list.

```
OEM_CARGO_CATALOG|1
C|54455354|5465737420636172676F|1.0|100|60
```

The example cargo ID is `TEST`, display name is `Test cargo`, unit mass is 1 kg, company purchase price is 100 credits, and company sale price is 60 credits. It matches the two-unit TEST stock in the sample `cargo_stock_495353.cfg`. These prices and the 10,000-credit example balance are test values; choose the opening balance and prices deliberately before using the economy in a live scenario.

## New Game / campaign reset

The portable core now defines and tests the allowlist of OEM campaign files that a future in-game New Game command may remove. The native MFD command is not yet exposed. Until it is implemented, do not manually delete individual files while Orbiter is running. The intended reset removes economy and event journals, generated cargo stock files, player station registrations, and cargo-service permissions while preserving account/catalogue definitions, module preferences, and the derived `spaceports.registry`. The reset must never touch Orbiter scenario files.

Do not change a service's opening stock configuration after transactions have been written to `economy.journal`, unless the journal is also intentionally reset. Recovery always replays the complete global journal over the configured opening stock.

## First market test

With the sample ISS stock file, create the two files above in `OEMData`, then restart the scenario so opening configuration is loaded. Dock to ISS and open `OEM Location Market` (key M). At a planetary base, land with ground contact within 5 km of the base and open the same MFD. BUY should reduce ISS TEST stock by one, add one TEST to the vessel manifest, and debit 100 credits from `company`. SELL should reverse the cargo quantities and credit 60 credits. Each operation is appended to the same `economy.journal` used by cargo exchanges. After a scenario restart, verify that balance, station stock, and vessel manifest are replayed consistently. For a planetary base, OEM creates `cargo_stock_<HEX base name>.cfg` automatically on the first confirmed landing. For example, base name `Brighton Beach` becomes `cargo_stock_4272696768746F6E204265616368.cfg` with header `OEM_CARGO_STOCK|1|4272696768746F6E204265616368`. To add opening stock, stop Orbiter and edit the generated file using `S|<HEX cargo ID>|<quantity>|<unit mass kg>` records before any transactions are written for that location. The market is ledger-only: it does not modify Orbiter's physical cargo or vessel mass.

## Independent objective missions

Open **OEM Independent Missions** with MFD key **I** while landed at a registered surface spaceport or docked to a registered economic station. Orbiter surface bases are discovered automatically. An orbital station must first be registered through OEM Economic Admin (key A); cargo-service permission and station registration are separate roles.

The board generates zero to three offers per simulation day. **VISIT** objectives require an actual Orbiter landing event at the exact named base and celestial body. **DOCK** objectives require an actual docking transition with the exact registered station. Merely entering the vicinity, opening the MFD, or pressing a button cannot complete an objective.

A company account named `company` in `accounts.cfg` is required before paid objective offers are shown or accepted. Rewards are credited automatically after verified completion. Deadlines use Orbiter simulation time. An accepted objective can be cancelled from the MFD without a monetary or reputation penalty; expired and cancelled objectives remain in the journal history.

Objective offers are saved in `objective_mission_boards.dat`. Accepted missions, verified completion evidence, expiry, cancellation, and exactly-once payouts are recorded in the shared `economy.journal`. The objective system is accounting-only: visit and docking missions do not create cargo, alter physical vessel mass, or modify Orbiter's docking/landing behaviour.

## Planetary spaceport registry

At simulation start, OEM enumerates all celestial bodies and all surface bases exposed by the Orbiter SDK. It writes a refreshed, versioned snapshot to `OEMData/spaceports.registry`. This file is generated data and is atomically replaced on the next simulation start.

Each surface-base record contains:
- stable OEM identifier `<body name>/<base name>`;
- exact Orbiter body and base names;
- base longitude and latitude in radians;
- parent body's mean radius and mass;
- derived surface gravity and escape velocity;
- every VTOL landing pad exposed by the SDK, including its coordinates, radius and whether an ILS transmitter is present.

The SDK's `oapiGetBasePadCount()` counts VTOL pads only, explicitly not runways. The current registry therefore does not claim runway counts, atmospheric composition, weather, local industry, or economic activity. Those require separate configuration parsing or future OEM-authored profiles; they are not fabricated from missing SDK data.

The registry is a mission-target catalogue, not a market stock file. It does not alter `cargo_stock_*.cfg`, the economy journal, or vessel cargo. Orbital stations are now represented in the shared economic-location model only when explicitly registered in-game through the OEM Economic Admin MFD. A vessel with cargo-service permission but without station registration remains a service vessel, not a station.
