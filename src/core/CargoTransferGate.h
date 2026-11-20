#ifndef OEM_CORE_CARGO_TRANSFER_GATE_H
#define OEM_CORE_CARGO_TRANSFER_GATE_H

#include "CargoManifest.h"

namespace oem {

// Facts sampled by the Orbiter bridge. A cargo-service docking flag may only
// be set by an adapter that recognizes the docked vessel as an OEM terminal.
struct CargoTransferObservation {
    std::string vesselId;
    bool landed;
    bool groundContact;
    bool surfaceLocationKnown;
    std::string surfaceBaseId;
    double surfaceBaseDistanceMetres;

    bool dockedToCargoService;
    std::string cargoServiceVesselId;
    CargoTransferObservation();
};

class CargoTransferGate {
public:
    Result authorize(const CargoTransfer& transfer,
                     const CargoTransferObservation& observation) const;
    Result apply(CargoManifest& manifest, const CargoTransfer& transfer,
                const CargoTransferObservation& observation) const;
};

} // namespace oem

#endif
