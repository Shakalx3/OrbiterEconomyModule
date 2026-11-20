#ifndef OEM_CORE_CARGO_TRANSFER_GATE_H
#define OEM_CORE_CARGO_TRANSFER_GATE_H
#include "CargoManifest.h"
#include "CargoServiceRegistry.h"
namespace oem {
struct CargoTransferObservation {
    std::string vesselId;
    bool landed;
    bool groundContact;
    bool surfaceLocationKnown;
    std::string surfaceBaseId;
    double surfaceBaseDistanceMetres;
    bool docked;
    std::string dockedVesselId;
    CargoTransferObservation();
};
class CargoTransferGate {
public:
    Result authorize(const CargoTransfer& transfer,const CargoTransferObservation& observation,
                     const CargoServiceRegistry& services) const;
    Result apply(CargoManifest& manifest,const CargoTransfer& transfer,
                 const CargoTransferObservation& observation,
                 const CargoServiceRegistry& services) const;
};
}
#endif
