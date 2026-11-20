# Economy rules — initial design

These are design constraints, not final balance values.

- Currency is represented as integer credits (no floating-point money).
- A market quote is a snapshot with a timestamp and station identifier.
- A purchase is one atomic operation: validate company funds and station stock, debit the company, decrement stock and add cargo. Stations and spaceports have unlimited funds and do not have finite-money ledger accounts.
- A sale validates cargo ownership and quantity, removes cargo from the vessel manifest, increases station stock and credits the company. Station payment capacity is unlimited.
- Every economic operation has a unique operation ID. Replaying it returns the prior result instead of applying it twice.
- Contract eligibility is evaluated from recorded simulation evidence, not from the requested UI action.
- Economy ticks use Orbiter simulation time. Paused simulation does not accrue production or time-based fees.
- Offline play is the default. No network service is required.

Starting capital, prices, margins, and progression thresholds remain configurable and will be balanced only after the first market loop is playable.

## Market catalog and quotes

The portable `CargoCatalog` stores explicit cargo definitions: stable cargo ID, display name, unit mass, station buy price and station sell price. Prices are integer credits; unit mass must be positive and finite; IDs are unique. No catalog entries or prices are invented as runtime defaults.

`MarketQuoteEngine` creates a timestamped quote snapshot for one exact cargo-service vessel. A BUY quote means the player buys from the station and therefore checks current station stock. A SELL quote means the player sells to the station and checks that accepting the quantity cannot overflow station stock. The quote captures station ID, cargo ID, side, quantity, unit price, total integer-credit value, observed station stock and simulation time. It does not reserve stock, move cargo, or transfer money. A later transaction must revalidate the quote against current state and commit company balance, station inventory and vessel manifest changes through one durable operation. No station account is required.

## Atomic market operation prototype

The portable `MarketTradeEngine` now applies a quoted BUY or SELL against candidate copies of the company ledger, station inventory and vessel manifest. BUY debits the company account and consumes station stock and adds to the vessel OEM manifest. SELL credits the company account and requires the manifest to contain the requested cargo. All candidate projections are published only if every validation succeeds. The operation ID is shared by the ledger and cargo transfer records, so an exact replay is idempotent and conflicting reuse is rejected.

This is currently an in-memory core transaction prototype. It is covered by portable tests but is not yet crash-durable and is not exposed in the Orbiter MFD. A unified durable market-trade journal and recovery path must be completed before enabling player-facing purchase/sale controls.

## Durable market trade journal

`MarketTradeJournalStore` persists a BUY/SELL as one append-only commit record. The record binds the operation to an exact station vessel ID and client vessel ID, and stores the company account ID, cargo ID, side, quantity, unit/total integer-credit price and simulation time. Station funds are not recorded. The current prototype journal is shared per station and records the exact client vessel on every trade, so that station stock and company trade history remain coherent across multiple ships. Because company balances are global across stations and cargo exchanges also change station stock, this per-station prototype is not yet sufficient as the final recovery authority. Recovery rebuilds a map of all vessel manifests touched by that station's trades. Append is flushed with `FlushFileBuffers` on Windows or `fsync` on POSIX before candidate ledger, station inventory and vessel manifest projections are published in memory. A failed append leaves all three live projections unchanged.

Recovery starts from explicitly configured opening account balances, explicit station stock and an empty manifest for the journal's exact vessel. Records replay into candidate copies; all three projections are published only if every complete record validates. An incomplete final line is discarded. A malformed complete line, wrong station/vessel identity, unavailable account, insufficient funds/stock or catalog mismatch rejects recovery without partially publishing state. The journal does not yet configure account opening balances or catalog data itself; those remain separate explicit inputs.


## Economy journal integration requirement

Cargo exchanges and future market trades share one ordered durable history. The native bridge now commits cargo exchanges to `OEMData/economy.journal`; the portable journal also supports market-trade records. Recovery starts from explicit opening account balances, cargo catalog and station stock configuration, then replays the global journal across all stations and vessels. Previous per-service exchange and per-vessel manifest journals were test artifacts and are intentionally ignored, not migrated. The original files are left untouched. The native Station Market MFD (key M) now exposes one-unit BUY/SELL operations at registered docked cargo services. It requires an explicitly configured `company` account and cargo catalog. Each trade appends to the same global journal and updates company balance, station stock, and the OEM vessel manifest atomically. Station funds are unlimited; only the company's account is debited for purchases and credited for sales. The MFD remains ledger-only and does not change Orbiter physical cargo or vessel mass.
