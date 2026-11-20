#include "CargoTransferGate.h"
#include "SurfaceDistance.h"
namespace oem {
CargoTransferObservation::CargoTransferObservation()
    : vesselId(),landed(false),groundContact(false),surfaceLocationKnown(false),
      surfaceBaseId(),surfaceBaseDistanceMetres(-1.0),docked(false),dockedVesselId() {}
Result CargoTransferGate::authorize(const CargoTransfer& transfer,
                                    const CargoTransferObservation& observation,
                                    const CargoServiceRegistry& services) const {
    if(transfer.vesselId.empty()||observation.vesselId.empty()||
       transfer.vesselId!=observation.vesselId)
        return Result::failure("CARGO_VESSEL_MISMATCH","Cargo transfer vessel does not match the observed vessel.");
    const bool verifiedSpaceport=observation.landed&&observation.groundContact&&
        observation.surfaceLocationKnown&&!observation.surfaceBaseId.empty()&&
        isWithinSpaceportRadius(observation.surfaceBaseDistanceMetres,5000.0)&&
        transfer.locationId==observation.surfaceBaseId;
    const bool verifiedCargoService=observation.docked&&
        services.isRegistered(observation.dockedVesselId)&&
        transfer.locationId==observation.dockedVesselId;
    if(!verifiedSpaceport&&!verifiedCargoService)
        return Result::failure("CARGO_LOCATION_UNVERIFIED",
            "Transfer requires verified landed ground contact at the named spaceport or docking to a registered OEM cargo service.");
    return Result::success();
}
Result CargoTransferGate::apply(CargoManifest& manifest,const CargoTransfer& transfer,
                                const CargoTransferObservation& observation,
                                const CargoServiceRegistry& services) const {
    Result allowed=authorize(transfer,observation,services);if(!allowed.ok)return allowed;
    if(transfer.direction==CARGO_LOAD)return manifest.load(transfer);
    if(transfer.direction==CARGO_UNLOAD)return manifest.unload(transfer);
    return Result::failure("CARGO_DIRECTION","Unknown cargo transfer direction.");
}
}
