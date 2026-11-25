#include "../../src/core/OpeningStock.h"
#include <cassert>
#include <cstdio>

static oem::CargoCatalog makeCatalog() {
    oem::CargoCatalog catalog;
    oem::CargoDefinition oxygen; oxygen.cargoId="oxygen"; oxygen.displayName="Oxygen";
    oxygen.unitMassKg=1.0; oxygen.buyPrice=10; oxygen.sellPrice=5;
    assert(catalog.add(oxygen).ok);
    oem::CargoDefinition water; water.cargoId="water"; water.displayName="Water";
    water.unitMassKg=2.0; water.buyPrice=8; water.sellPrice=4;
    assert(catalog.add(water).ok);
    return catalog;
}
static oem::OpeningStockProfile makeProfile() {
    oem::OpeningStockProfile p; p.bodyName="Moon"; p.enabled=true;
    oem::OpeningStockRule a; a.cargoId="oxygen"; a.minimumQuantity=10; a.maximumQuantity=30;
    oem::OpeningStockRule b; b.cargoId="water"; b.minimumQuantity=0; b.maximumQuantity=20;
    p.rules.push_back(a); p.rules.push_back(b); return p;
}
int main() {
    oem::CargoCatalog catalog=makeCatalog();
    oem::OpeningStockProfile profile=makeProfile();
    oem::OpeningStockContext context; context.locationId="Tranquility";
    context.bodyName="Moon"; context.seed=12345;
    oem::CargoServiceInventory first("Tranquility"), second("Tranquility");
    oem::OpeningStockGenerator generator;
    assert(generator.generate(profile,context,catalog,first).ok);
    assert(generator.generate(profile,context,catalog,second).ok);
    assert(first.quantity("oxygen")>=10 && first.quantity("oxygen")<=30);
    assert(first.quantity("water")<=20);
    assert(first.quantity("oxygen")==second.quantity("oxygen"));
    assert(first.quantity("water")==second.quantity("water"));
    assert(first.lines().size()==second.lines().size());

    oem::OpeningStockContext other=context; other.locationId="Shackleton";
    oem::CargoServiceInventory otherStock("Shackleton");
    assert(generator.generate(profile,other,catalog,otherStock).ok);
    oem::CargoServiceInventory occupied("Tranquility");
    assert(occupied.setInitialStock("oxygen",1,1.0).ok);
    assert(!generator.generate(profile,context,catalog,occupied).ok);

    oem::OpeningStockProfile bad=profile; bad.bodyName="Mars";
    oem::CargoServiceInventory wrongBody("Tranquility");
    assert(!generator.generate(bad,context,catalog,wrongBody).ok);
    oem::OpeningStockProfile unknown=profile;
    oem::OpeningStockRule invalid; invalid.cargoId="unlisted"; invalid.minimumQuantity=1; invalid.maximumQuantity=2;
    unknown.rules.push_back(invalid);
    oem::CargoServiceInventory invalidStock("Tranquility");
    assert(!generator.generate(unknown,context,catalog,invalidStock).ok);

    const std::string path="OEMOpeningStockTests.cfg"; std::remove(path.c_str());
    oem::CargoServiceInventory initialized("Tranquility");
    assert(generator.initialize(path,&profile,context,catalog,false,initialized).ok);
    const long long savedOxygen=initialized.quantity("oxygen");
    oem::CargoServiceInventory reload("Tranquility");
    assert(generator.initialize(path,&profile,context,catalog,false,reload).ok);
    assert(reload.quantity("oxygen")==savedOxygen);
    assert(!generator.initialize("OEMOpeningStockMissing.cfg",&profile,context,catalog,true,reload).ok);
    std::remove(path.c_str());
    return 0;
}
