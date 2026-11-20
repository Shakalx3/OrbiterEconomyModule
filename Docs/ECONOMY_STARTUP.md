# Starting a clean OEM economy

The native bridge now uses one file, `OEMData/economy.journal`, for all future cargo exchanges and market trades. The older `cargo_exchanges` and `cargo_manifests` test journals are ignored and are not imported.

## Opening state

The following files are read beside the module, under `OEMData`:

- `cargo_services.cfg`: explicit list of trusted service-vessel names.
- `cargo_stock_<HEX vessel ID>.cfg`: explicit opening stock for each registered service vessel. A missing stock file means an empty inventory.
- `accounts.cfg`: optional until market trading is enabled. If absent, the company ledger starts with no accounts and market trades cannot be executed.
- `cargo_catalog.cfg`: optional until market trading is enabled. If absent, there are no defined tradeable cargo types.

The native Station Market MFD (key M) uses the company account ID `company`. Market trades are enabled only when `accounts.cfg` defines that account and `cargo_catalog.cfg` defines at least one item. Trade buttons buy or sell one unit of the first catalog item for which the station or vessel has available quantity.

No starting money, cargo definitions, prices, or stock quantities are generated automatically.

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

## Reset policy

To intentionally reset a test economy, stop Orbiter and back up or remove only `OEMData/economy.journal`. Then review the explicit opening configuration files. The old test journals may be kept or deleted separately; they are not read by the new bridge.

Do not change a service's opening stock configuration after transactions have been written to `economy.journal`, unless the journal is also intentionally reset. Recovery always replays the complete global journal over the configured opening stock.

## First market test

With the sample ISS stock file, create the two files above in `OEMData`, then restart the scenario so opening configuration is loaded. Dock to ISS and open `OEM Station Market` (key M). BUY should reduce ISS TEST stock by one, add one TEST to the vessel manifest, and debit 100 credits from `company`. SELL should reverse the cargo quantities and credit 60 credits. Each operation is appended to the same `economy.journal` used by cargo exchanges. After a scenario restart, verify that balance, station stock, and vessel manifest are replayed consistently. The market is ledger-only: it does not modify Orbiter's physical cargo or vessel mass.
