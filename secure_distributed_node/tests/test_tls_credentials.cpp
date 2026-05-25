#include "TlsCredentials.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

static const std::filesystem::path TEST_CERTS{TEST_CERTS_DIR};

class TlsCredentialsTest : public ::testing::Test {
protected:
    TlsCredentials creds_;
    void SetUp() override {
        creds_ = TlsCredentials::LoadFromDir(TEST_CERTS, "worker1");
    }
};

TEST_F(TlsCredentialsTest, LoadFromDir_FindsAllThreeFiles) {
    // SetUp succeeded without exception — all 3 PEM files loaded
    EXPECT_FALSE(creds_.ca_cert.empty());
    EXPECT_FALSE(creds_.node_cert.empty());
    EXPECT_FALSE(creds_.node_key.empty());
}

TEST_F(TlsCredentialsTest, ServerCredentials_NotNull) {
    EXPECT_NE(creds_.ServerCredentials(), nullptr);
}

TEST_F(TlsCredentialsTest, ChannelCredentials_NotNull) {
    EXPECT_NE(creds_.ChannelCredentials(), nullptr);
}

TEST_F(TlsCredentialsTest, LoadFromDir_MissingKey_Throws) {
    auto bad_dir = std::filesystem::temp_directory_path() / "bad_certs_test";
    std::filesystem::create_directories(bad_dir);
    // Copy ca.crt and worker1.crt but NOT .key
    std::filesystem::copy_file(TEST_CERTS / "ca.crt", bad_dir / "ca.crt",
        std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file(TEST_CERTS / "worker1.crt", bad_dir / "worker1.crt",
        std::filesystem::copy_options::overwrite_existing);
    EXPECT_THROW(TlsCredentials::LoadFromDir(bad_dir, "worker1"), std::runtime_error);
    std::filesystem::remove_all(bad_dir);
}
