#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

struct TlsCredentials {
    std::string ca_cert;    // CA root PEM — verifies peer
    std::string node_cert;  // this node's cert PEM
    std::string node_key;   // this node's private key PEM

    static TlsCredentials LoadFromDir(const std::filesystem::path& dir,
                                      const std::string& node_name);

    std::shared_ptr<grpc::ServerCredentials>  ServerCredentials() const;
    std::shared_ptr<grpc::ChannelCredentials> ChannelCredentials() const;
};
