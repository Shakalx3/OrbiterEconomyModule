#include "../../src/core/SpaceportRegistry.h"
#include <cassert>
#include <cstdio>
#include <fstream>
int main(){
 const std::string path="OEMSpaceportRegistryTests.cfg";std::remove(path.c_str());
 oem::SpaceportRegistry source;oem::SpaceportRecord earth;earth.id="Earth/Canaveral";earth.bodyName="Earth";earth.baseName="Canaveral";earth.longitudeRad=-1.4;earth.latitudeRad=0.5;earth.bodyRadiusMetres=6371000;earth.bodyMassKg=5.972e24;earth.surfaceGravityMps2=9.81;earth.escapeVelocityMps=11186;
 assert(source.add(earth).ok);oem::SpaceportPadRecord pad;pad.index=0;pad.longitudeRad=-1.4001;pad.latitudeRad=0.5001;pad.radiusMetres=6371000;pad.hasIls=true;assert(source.addPad(earth.id,pad).ok);
 oem::SpaceportRecord moon;moon.id="Moon/Tranquility";moon.bodyName="Moon";moon.baseName="Tranquility";moon.longitudeRad=0.2;moon.latitudeRad=-0.1;moon.bodyRadiusMetres=1737400;moon.bodyMassKg=7.342e22;moon.surfaceGravityMps2=1.62;moon.escapeVelocityMps=2376;assert(source.add(moon).ok);
 oem::SpaceportRegistryConfigStore store;assert(store.save(path,source).ok);
 oem::SpaceportRegistry loaded;assert(store.load(path,loaded).ok);assert(loaded.entries().size()==2);
 const oem::SpaceportRecord* e=loaded.find("Earth/Canaveral");assert(e&&e->pads.size()==1&&e->pads[0].hasIls);
 assert(loaded.find("Moon/Tranquility")!=0);
 {std::ofstream out(path.c_str(),std::ios::app|std::ios::binary);out<<"BAD|record\n";}
 oem::SpaceportRegistry unchanged;assert(unchanged.add(moon).ok);assert(!store.load(path,unchanged).ok&&unchanged.entries().size()==1);
 oem::SpaceportRecord duplicate=moon;assert(!source.add(duplicate).ok);
 std::remove(path.c_str());return 0;
}
