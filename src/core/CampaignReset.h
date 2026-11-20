#ifndef OEM_CORE_CAMPAIGN_RESET_H
#define OEM_CORE_CAMPAIGN_RESET_H

#include <string>

namespace oem {

// Classifies OEMData filenames that belong to a playthrough. The native
// filesystem layer may delete only files accepted by this allowlist.
// Module settings, catalogues and Orbiter's derived spaceport registry are
// deliberately outside the campaign boundary.
class CampaignResetPolicy {
public:
    static bool isCampaignOwnedFileName(const std::string& fileName);
};

} // namespace oem

#endif
