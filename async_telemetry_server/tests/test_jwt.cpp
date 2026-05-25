#include <gtest/gtest.h>
#include "JwtValidator.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <jwt-cpp/jwt.h>
#pragma GCC diagnostic pop

#include <cstdlib>
#include <chrono>
#include <string>

using namespace ats;

static const char* SECRET = "test_secret_key_for_unit_tests";
static const char* ISSUER = "https://auth.nexburg.internal";

// Helper: build a JWT token with jwt-cpp
static std::string make_token(
        const std::string& secret,
        const std::string& issuer,
        const std::string& subject,
        int exp_offset_s = 3600) {
    auto now = std::chrono::system_clock::now();
    return jwt::create()
        .set_issuer(issuer)
        .set_subject(subject)
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::seconds{exp_offset_s})
        .sign(jwt::algorithm::hs256{secret});
}

class JwtTest : public ::testing::Test {
protected:
    void SetUp() override {
        ::setenv("JWT_SECRET", SECRET, 1);
    }
    JwtValidator v;
};

TEST_F(JwtTest, ValidTokenPasses) {
    auto token = make_token(SECRET, ISSUER, "client_42");
    auto res = v.validate(token);
    EXPECT_TRUE(res.ok);
    EXPECT_EQ(res.subject, "client_42");
    EXPECT_TRUE(res.error_detail.empty());
}

TEST_F(JwtTest, ExpiredTokenFails) {
    // exp = 60 seconds in the past (well outside 30s leeway)
    auto token = make_token(SECRET, ISSUER, "client_x", -60);
    auto res = v.validate(token);
    EXPECT_FALSE(res.ok);
    EXPECT_FALSE(res.error_detail.empty());
}

TEST_F(JwtTest, WrongSignatureFails) {
    auto token = make_token("wrong_secret_entirely", ISSUER, "client_x");
    auto res = v.validate(token);
    EXPECT_FALSE(res.ok);
    EXPECT_FALSE(res.error_detail.empty());
}

TEST_F(JwtTest, WrongIssuerFails) {
    auto token = make_token(SECRET, "https://evil.issuer.example", "client_x");
    auto res = v.validate(token);
    EXPECT_FALSE(res.ok);
    EXPECT_FALSE(res.error_detail.empty());
}

TEST_F(JwtTest, EmptySubjectFails) {
    auto token = make_token(SECRET, ISSUER, "");
    auto res = v.validate(token);
    EXPECT_FALSE(res.ok);
    EXPECT_FALSE(res.error_detail.empty());
}
