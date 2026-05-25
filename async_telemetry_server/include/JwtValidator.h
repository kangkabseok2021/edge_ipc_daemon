#pragma once
#include <string>

namespace ats {

struct ValidationResult {
    bool        ok{false};
    std::string subject;
    std::string error_detail;
};

class JwtValidator {
public:
    // Validates Bearer token. Reads JWT_SECRET from env.
    [[nodiscard]] ValidationResult validate(const std::string& token) const;
};

} // namespace ats
