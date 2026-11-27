#ifndef OEM_CORE_ECONOMIC_AGENT_CONFIG_H
#define OEM_CORE_ECONOMIC_AGENT_CONFIG_H
#include "EconomicAgent.h"
#include <string>
namespace oem {
class EconomicAgentConfigStore {
public:
    Result load(const std::string& path, EconomicAgentBook& book) const;
    Result save(const std::string& path, const EconomicAgentBook& book) const;
};
}
#endif
