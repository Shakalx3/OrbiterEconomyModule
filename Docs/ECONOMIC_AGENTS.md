# OEM abstract economic agents

## Purpose

Economic agents are simulated organizations, not Orbiter vessels. They trade with the stock of registered economic locations (orbital stations and planetary spaceports) without creating physical ships, docking events, or player cargo manifests.

## First-run behavior

When `OEMData/economic_agents.cfg` is absent, OEM creates a starter profile if the economy has at least two registered locations and a cargo catalogue:

- One `OEM Interplanetary Trader` with the `COMPETITOR` role.
- Operating access to all registered economic locations.
- Opening balance: 50,000 credits.
- One policy for each cargo definition, with a target inventory of 20 units.
- Activity interval: 86,400 simulation seconds (one simulation day).

The starter profile is written once. If fewer than two locations or no cargo definitions exist, an empty agent configuration is created instead.

## Configuration format

The file starts with:

```
OEM_ECONOMIC_AGENTS|1
```

Records are pipe-delimited. Text identifiers and names are encoded as hexadecimal UTF-8 bytes.

- `A|agentId|displayName|role|homeLocation|balance|unlimitedFunds|activityInterval|lastActivityTime|enabled`
- `L|agentId|operatingLocation`
- `P|agentId|cargoId|targetQuantity|maximumBuyPrice|minimumSellPrice`
- `I|agentId|cargoId|quantity|originLocation|unitCost`

Role values: `1=SUPPLIER`, `2=CUSTOMER`, `3=CARRIER`, `4=COMPETITOR`. Boolean fields are `0` or `1`. Use `-` for an empty opening cargo-origin location.

`activityInterval` is measured in simulation seconds. Credits and quantities are integer values. Cargo mass comes from the cargo catalogue, not from the agent file.

## Trading rules

- A buyer chooses the operating location with the lowest current dynamic buy quote, provided the location has stock and the agent can afford the purchase.
- A supplier can sell only cargo already present in its agent inventory.
- A competitor or carrier can buy and then sell at a different operating location.
- A sale is considered only when the destination's current sell-to-station quote covers the recorded purchase cost and the configured minimum sale price.
- The station inventory is debited/credited by a normal cargo inventory transfer. Agent inventory and balance are updated in the same journalled transaction.
- No physical Orbiter vessel is created. An abstract sale is not a claim that a vessel flew or docked.

The current route model uses the configured activity interval between trade actions. Distance-aware travel time, transport capacity, a global intensity setting, and an in-game activity screen are not implemented yet.

## Persistence and recovery

`economic_agents.cfg` is the opening profile, not a continuously rewritten snapshot. Do not replace it with the current in-memory balances after trades: the global `economy.journal` is authoritative for committed agent trades. On startup, OEM loads the opening profile and replays agent trade records to reconstruct agent balance, inventory, cargo origin/cost basis, and last activity time together with station stock.

Keep the configuration and global journal together when backing up or moving a save. Deleting the journal while keeping a post-trade stock/configuration set is not a supported reset procedure.
