#include "JwtValidator.h"

namespace ats {

ValidationResult JwtValidator::validate(const std::string& /*token*/) const {
    return {false, "", "not implemented"};
}

} // namespace ats
