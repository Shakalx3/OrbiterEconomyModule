# Starting a clean OEM economy

The native bridge now uses one file, `OEMData/economy.journal`, for all future cargo exchanges and market trades. The older `cargo_exchanges` and `cargo_manifests` test journals are ignored and are not imported.

## Opening state

The following files are read beside the module, under `OEMData`:

- `economic_stations.registry`: player-managed orbital station identities. Register or remove the focused vessel in-game using the OEM Economic Admin MFD (key A). OEM creates and maintains this file; do not edit it manually.
- `cargo_services.cfg`: trusted cargo-service permissions, maintained in-game from the same Admin MFD. A cargo-service vessel is not automatically an orbital station; these are independent roles.
- `cargo_stock_<HEX location ID>.cfg`: explicit opening stock for each registered station vessel or planetary surface base. For a planetary base, OEM automatically creates this file on its first confirmed landing, using the exact Orbiter base name; it starts empty unless a matching primary-resource source defines an explicit one-time opening quantity. A configured opening quantity can also initialize an already-existing but still-empty file, provided that the location has no global-journal history. Non-empty or journal-backed stock is never overwritten or refilled.
- `accounts.cfg`: optional until market trading is enabled. If absent, the company ledger starts with no accounts and market trades cannot be executed.
- `cargo_catalog.cfg`: optional until market trading is enabled. If absent, there are no defined tradeable cargo types.
- `objective_mission_boards.dat`: generated, versioned daily offers for independent visit and docking missions. OEM creates and atomically updates it; do not edit it manually. Accepted mission state and payouts are stored in `economy.journal`, not in this board file.
- `mission_failure.cfg`: configurable expiry penalties shared by delivery contracts and independent VISIT/DOCK missions. OEM creates it with defaults if absent and preserves existing values.

The native Location Market MFD (key M) uses the company account ID `company`. Market trades are enabled only when `accounts.cfg` defines that account and `cargo_catalog.cfg` defines at least one item. Trade buttons buy or sell one unit of the first catalog item for which the station or vessel has available quantity.

No starting money, cargo definitions, prices, or production sources are invented. Automatic location registration creates an empty stock file; only an explicit `openingStock` in a matching `primary_resources.cfg` source may seed that newly created file. Existing stock files, including intentionally empty ones, are never refilled.

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

## Primary resources and background production

`OEMData/primary_resources.cfg` is created automatically with this header when missing:

```
OEM_PRIMARY_RESOURCES|2
```

Add one source record per cargo type and economic location:

```
S|<HEX source ID>|<HEX location ID>|<HEX cargo ID>|<unit mass kg>|<units per cycle>|<cycle seconds>|<maximum local stock>|<last production time>|<enabled 0/1>|<opening stock>
```

Example only, using the sample `TEST` cargo and a registered economic station whose storage ID is `ISS`:

```
OEM_PRIMARY_RESOURCES|2
S|746573742D7072696D617279|495353|54455354|1.0|20|86400|150|0|1|120
```

This example seeds 120 TEST units once when the `ISS` stock inventory is first eligible (empty and with no journal history), then clears the `openingStock` field in `primary_resources.cfg` and produces up to 20 units per 86,400 seconds of Orbiter simulation time, stopping at 150 units. Replace the location ID with the exact storage ID of a registered economic location and ensure the cargo ID and unit mass match `cargo_catalog.cfg`. A surface-base storage ID must be unambiguous; duplicate base names on different bodies cannot share a primary source under the legacy stock-file naming scheme.

`openingStock` is a one-time initialization quantity. OEM applies it only when the location inventory is empty and has no journal history, then clears that source's `openingStock` value in the versioned config. If the stock already contains cargo or the journal references that location, OEM preserves the existing state and consumes the one-time opening value without adding cargo. If a stock file is missing while `economy.journal` contains transactions for that location, OEM refuses to invent a replacement opening baseline. The journal restores each source's last committed production timestamp before the next production cycle, so a crash after a durable production commit cannot create the same cycle twice. Production scheduling uses absolute Orbiter MJD-derived simulation seconds (not wall-clock time or the scenario-relative `simt` counter), so the production clock continues correctly after a scenario save/reload. A zero `last production time` is initialized to the current Orbiter MJD on the first active simulation step. Production records update the location inventory and journal; they do not create physical cargo in an Orbiter vessel.

Version-1 source files remain readable; their opening-stock value defaults to zero. OEM writes version 2 when it saves source scheduling state. Stop Orbiter before editing this file. Do not change source identity, location, cargo, or unit mass while the global journal contains history for that source.

## Mission expiry penalties

The generated `OEMData/mission_failure.cfg` controls penalties for accepted delivery contracts and independent **VISIT**/**DOCK** missions that pass their deadline:

```
OEM_MISSION_FAILURE|1
P|1000|-5
```

The first value is a percentage in basis points: `1000` means 10% of the mission reward, `2500` means 25%, and `0` disables the money fee. The fee is capped at the company's currently available positive balance; an expired mission still records its reputation penalty when the account has no credits. The second value is the reputation change and must be between `-1` and `-100`; the default `-5` reduces reputation by five points. The same policy applies to both mission systems.

Only deadline expiry is penalized. Cancelling an accepted mission or delivery contract remains penalty-free. Expiry, the exact fee and reputation change are committed together to `economy.journal`, so replay after a restart does not charge twice. The policy file is created only when missing; existing user settings are not overwritten. If its format or values are invalid, correct the file before starting the economy module.

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
