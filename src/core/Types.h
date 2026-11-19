#ifndef OEM_CORE_TYPES_H
#define OEM_CORE_TYPES_H

#include <string>

namespace oem {

typedef long long Credits;
typedef long long SimulationSeconds;

struct Result {
    bool ok;
    std::string code;
    std::string message;

    Result() : ok(false), code("NOT_INITIALIZED"), message() {}
    Result(bool success, const std::string& resultCode, const std::string& resultMessage)
        : ok(success), code(resultCode), message(resultMessage) {}

    static Result success() { return Result(true, "OK", ""); }
    static Result failure(const std::string& code, const std::string& message) {
        return Result(false, code, message);
    }
};

} // namespace oem

#endif
