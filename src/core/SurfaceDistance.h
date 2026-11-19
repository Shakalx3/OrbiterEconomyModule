#ifndef OEM_CORE_SURFACE_DISTANCE_H
#define OEM_CORE_SURFACE_DISTANCE_H

#include <cmath>

namespace oem {

// Great-circle distance between two points on the same spherical body.
// Coordinates are radians; radius is the body's mean radius in metres.
inline double surfaceDistanceMetres(double lat1, double lon1,
                                    double lat2, double lon2,
                                    double bodyRadiusMetres) {
    if (!(bodyRadiusMetres > 0.0) ||
        !std::isfinite(lat1) || !std::isfinite(lon1) ||
        !std::isfinite(lat2) || !std::isfinite(lon2) ||
        !std::isfinite(bodyRadiusMetres)) return -1.0;

    const double dLat = lat2 - lat1;
    const double dLon = lon2 - lon1;
    const double sinLat = std::sin(dLat * 0.5);
    const double sinLon = std::sin(dLon * 0.5);
    double a = sinLat * sinLat + std::cos(lat1) * std::cos(lat2) * sinLon * sinLon;
    if (a < 0.0) a = 0.0;
    if (a > 1.0) a = 1.0;
    return 2.0 * bodyRadiusMetres * std::asin(std::sqrt(a));
}

inline bool isWithinSpaceportRadius(double distanceMetres, double radiusMetres = 5000.0) {
    return std::isfinite(distanceMetres) && distanceMetres >= 0.0 &&
           std::isfinite(radiusMetres) && radiusMetres >= 0.0 &&
           distanceMetres <= radiusMetres;
}

} // namespace oem

#endif
