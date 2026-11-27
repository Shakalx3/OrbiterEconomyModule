#include "../../src/core/ShipOwnership.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>

int main(){
    oem::ShipCatalog catalog;
    oem::ShipDefinition dg;dg.className="DeltaGlider";dg.displayName="Delta-Glider";dg.price=250000;dg.orbitalDelivery=true;dg.surfaceDelivery=true;
    assert(catalog.add(dg).ok);assert(!catalog.add(dg).ok);
    oem::ShipDefinition station;station.className="OrbitalTender";station.displayName="Orbital Tender";station.price=100000;station.orbitalDelivery=true;
    assert(catalog.add(station).ok);
    oem::ShipPurchaseQuote quote;
    assert(oem::createShipPurchaseQuote(catalog,"buy-001","DeltaGlider","ISS",oem::SHIP_DELIVERY_ORBITAL,300000,1234,quote).ok);
    assert(quote.price==250000&&quote.className=="DeltaGlider"&&quote.locationId=="ISS");
    assert(!oem::createShipPurchaseQuote(catalog,"buy-002","OrbitalTender","Cape",oem::SHIP_DELIVERY_SURFACE,300000,1234,quote).ok);
    assert(!oem::createShipPurchaseQuote(catalog,"buy-003","DeltaGlider","ISS",oem::SHIP_DELIVERY_ORBITAL,200000,1234,quote).ok);
    assert(!oem::createShipPurchaseQuote(catalog,"buy-004","MissingClass","ISS",oem::SHIP_DELIVERY_ORBITAL,300000,1234,quote).ok);

    const std::string path="oem_player_fleet_test.registry";std::remove(path.c_str());std::remove((path+".tmp").c_str());
    oem::PlayerFleetRegistry fleet;assert(!fleet.isOwned("OEM-001"));
    oem::OwnedVesselRecord r;r.purchaseId="buy-001";r.vesselId="OEM-001";r.className="DeltaGlider";r.acquisitionLocationId="ISS";r.acquisitionPrice=250000;r.acquiredAt=1234;
    assert(fleet.registerPurchase(r).ok);assert(fleet.isOwned("OEM-001"));
    assert(fleet.registerPurchase(r).ok); // exact replay is idempotent
    oem::OwnedVesselRecord conflict=r;conflict.vesselId="OEM-002";assert(!fleet.registerPurchase(conflict).ok);
    conflict=r;conflict.purchaseId="buy-002";assert(!fleet.registerPurchase(conflict).ok);
    assert(fleet.save(path).ok);
    oem::PlayerFleetRegistry restored;assert(restored.load(path).ok&&restored.vessels().size()==1);
    oem::OwnedVesselRecord found;assert(restored.findByPurchase("buy-001",found)&&found.vesselId=="OEM-001");
    assert(restored.findByVessel("OEM-001",found)&&found.acquisitionPrice==250000);
    {std::ofstream out(path.c_str(),std::ios::binary|std::ios::trunc);out<<"OEM_PLAYER_FLEET|1\nV|GG|4F454D|44656C7461476C69646572|495353|10|2\n";}
    assert(!restored.load(path).ok);assert(restored.isOwned("OEM-001")); // failed load is non-destructive
    assert(restored.removePurchase("buy-001")&&!restored.isOwned("OEM-001"));
    std::remove(path.c_str());std::remove((path+".tmp").c_str());return 0;
}
