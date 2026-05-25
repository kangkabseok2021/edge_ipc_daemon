#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <jwt-cpp/jwt.h>
#pragma GCC diagnostic pop

#include "JwtValidator.h"
#include <cstdlib>

namespace ats {

static constexpr auto ISSUER   = "https://auth.nexburg.internal";
static constexpr int  LEEWAY_S = 30;

ValidationResult JwtValidator::validate(const std::string& token) const {
    const char* env_secret = std::getenv("JWT_SECRET");
    if (!env_secret)
        return {false, "", "JWT_SECRET environment variable not set"};

    std::string secret{env_secret};

    try {
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret})
            .with_issuer(ISSUER)
            .leeway(static_cast<std::size_t>(LEEWAY_S));

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

        std::string sub = decoded.get_subject();
        if (sub.empty())
            return {false, "", "empty subject claim"};

        return {true, sub, ""};

    } catch (const std::exception& e) {
        return {false, "", e.what()};
    }
}

} // namespace ats
