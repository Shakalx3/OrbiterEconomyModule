# Starting a clean OEM economy

The native bridge now uses one file, `OEMData/economy.journal`, for all future cargo exchanges and market trades. The older `cargo_exchanges` and `cargo_manifests` test journals are ignored and are not imported.

## Opening state

The following files are read beside the module, under `OEMData`:

- `cargo_services.cfg`: explicit list of trusted service-vessel names.
- `cargo_stock_<HEX vessel ID>.cfg`: explicit opening stock for each registered service vessel. A missing stock file means an empty inventory.
- `accounts.cfg`: optional until market trading is enabled. If absent, the company ledger starts with no accounts and market trades cannot be executed.
- `cargo_catalog.cfg`: optional until market trading is enabled. If absent, there are no defined tradeable cargo types.

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
C|4F3258|4F787967656E|10.0|100|60
```

The example cargo ID is `O2X`, display name is `Oxygen`, unit mass is 10 kg, company purchase price is 100 credits, and company sale price is 60 credits. Adjust these values before using the item in a live economy.

## Reset policy

To intentionally reset a test economy, stop Orbiter and back up or remove only `OEMData/economy.journal`. Then review the explicit opening configuration files. The old test journals may be kept or deleted separately; they are not read by the new bridge.

Do not change a service's opening stock configuration after transactions have been written to `economy.journal`, unless the journal is also intentionally reset. Recovery always replays the complete global journal over the configured opening stock.
