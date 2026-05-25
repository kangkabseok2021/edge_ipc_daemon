#include "TlsCredentials.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

static std::string ReadFile(const std::filesystem::path& p) {
    std::ifstream f(p);
    if (!f) throw std::runtime_error("TlsCredentials: cannot open " + p.string());
    return {std::istreambuf_iterator<char>(f), {}};
}

TlsCredentials TlsCredentials::LoadFromDir(const std::filesystem::path& dir,
                                            const std::string& node_name) {
    TlsCredentials c;
    c.ca_cert   = ReadFile(dir / "ca.crt");
    c.node_cert = ReadFile(dir / (node_name + ".crt"));
    c.node_key  = ReadFile(dir / (node_name + ".key"));
    return c;
}

std::shared_ptr<grpc::ServerCredentials> TlsCredentials::ServerCredentials() const {
    grpc::SslServerCredentialsOptions opts(GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY);
    opts.pem_root_certs = ca_cert;
    opts.pem_key_cert_pairs.push_back({node_key, node_cert});
    return grpc::SslServerCredentials(opts);
}

std::shared_ptr<grpc::ChannelCredentials> TlsCredentials::ChannelCredentials() const {
    grpc::SslCredentialsOptions opts;
    opts.pem_root_certs = ca_cert;
    opts.pem_private_key = node_key;
    opts.pem_cert_chain  = node_cert;
    return grpc::SslCredentials(opts);
}
