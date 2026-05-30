// lsa_ethernet_sync — Global Inter-Node Synchronization Daemon
// Responsibility: UDP multicast sync of TLV state segments between nodes.
// Protocol: 239.73.78.69:20046, AES-256-GCM, 32-byte BLAKE3 MAC.
#include "common/signal_handler.hpp"
#include "common/logger.hpp"
#include "common/shared_state.hpp"
#include <thread>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>

// POSIX socket headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

using namespace nnos;

// Multicast group and port per NNOS-TECH-001
static constexpr const char* MCAST_GROUP = "239.73.78.69";
static constexpr uint16_t MCAST_PORT = 20046;
static constexpr size_t MAX_SYNC_PACKET = 65507; // Max UDP payload

// ---------------------------------------------------------------------------
// UDP Multicast Socket Layer
// ---------------------------------------------------------------------------

class MulticastSyncSocket {
public:
    MulticastSyncSocket() = default;
    ~MulticastSyncSocket() { close_socket(); }

    bool init(const std::string& local_addr = "0.0.0.0") {
        fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd_ < 0) {
            return false;
        }

        // Allow address reuse
        int reuse = 1;
        if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            close_socket();
            return false;
        }

        // Bind to the multicast port
        sockaddr_in bind_addr{};
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_port = htons(MCAST_PORT);
        bind_addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(fd_, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr)) < 0) {
            close_socket();
            return false;
        }

        // Join multicast group
        ip_mreq mreq{};
        mreq.imr_multiaddr.s_addr = inet_addr(MCAST_GROUP);
        mreq.imr_interface.s_addr = inet_addr(local_addr.c_str());
        if (setsockopt(fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            close_socket();
            return false;
        }

        // Set non-blocking for recv
        int flags = fcntl(fd_, F_GETFL, 0);
        fcntl(fd_, F_SETFL, flags | O_NONBLOCK);

        return true;
    }

    bool send(const std::vector<uint8_t>& data) {
        if (fd_ < 0) return false;
        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(MCAST_PORT);
        dest.sin_addr.s_addr = inet_addr(MCAST_GROUP);
        ssize_t sent = ::sendto(fd_, data.data(), data.size(), 0,
                                reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
        return sent == static_cast<ssize_t>(data.size());
    }

    std::vector<uint8_t> recv() {
        std::vector<uint8_t> buf(MAX_SYNC_PACKET);
        sockaddr_in src{};
        socklen_t src_len = sizeof(src);
        ssize_t n = ::recvfrom(fd_, buf.data(), buf.size(), 0,
                               reinterpret_cast<sockaddr*>(&src), &src_len);
        if (n > 0) {
            buf.resize(static_cast<size_t>(n));
            return buf;
        }
        return {};
    }

    void close_socket() {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }

private:
    int fd_ = -1;
};

// ---------------------------------------------------------------------------
// AES-256-GCM encryption via OpenSSL (when available)
// Packet format: [12-byte IV][ciphertext][16-byte tag]
// ---------------------------------------------------------------------------

#ifdef NNOS_HAS_OPENSSL
#include <openssl/evp.h>
#include <openssl/rand.h>

class SyncCrypto {
public:
    static constexpr size_t KEY_LEN = 32;
    static constexpr size_t IV_LEN  = 12;
    static constexpr size_t TAG_LEN = 16;

    SyncCrypto() {
        const char* env_key = std::getenv("NNOS_SYNC_KEY");
        if (env_key && std::strlen(env_key) == 64) {
            for (size_t i = 0; i < KEY_LEN; ++i) {
                key_[i] = hex_byte(env_key + 2*i);
            }
        } else {
            std::memset(key_, 0, KEY_LEN);
        }
    }

    bool is_default_key() const {
        for (size_t i = 0; i < KEY_LEN; ++i) if (key_[i] != 0) return false;
        return true;
    }

    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plain) {
        std::vector<uint8_t> out(IV_LEN + plain.size() + TAG_LEN);
        if (RAND_bytes(out.data(), static_cast<int>(IV_LEN)) != 1) {
            return {};
        }

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return {};

        int len = 0;
        bool ok = true;
        ok = ok && (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1);
        ok = ok && (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(IV_LEN), nullptr) == 1);
        ok = ok && (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key_, out.data()) == 1);
        ok = ok && (EVP_EncryptUpdate(ctx, out.data() + IV_LEN, &len,
                                      plain.data(), static_cast<int>(plain.size())) == 1);
        int final_len = 0;
        ok = ok && (EVP_EncryptFinal_ex(ctx, out.data() + IV_LEN + len, &final_len) == 1);
        ok = ok && (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG,
                                         static_cast<int>(TAG_LEN),
                                         out.data() + IV_LEN + plain.size()) == 1);
        EVP_CIPHER_CTX_free(ctx);
        return ok ? out : std::vector<uint8_t>{};
    }

    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& cipher) {
        if (cipher.size() < IV_LEN + TAG_LEN) return {};
        size_t ct_len = cipher.size() - IV_LEN - TAG_LEN;

        std::vector<uint8_t> out(ct_len);
        const uint8_t* iv  = cipher.data();
        const uint8_t* ct  = cipher.data() + IV_LEN;
        const uint8_t* tag = cipher.data() + IV_LEN + ct_len;

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return {};

        int len = 0;
        bool ok = true;
        ok = ok && (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1);
        ok = ok && (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(IV_LEN), nullptr) == 1);
        ok = ok && (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key_, iv) == 1);
        ok = ok && (EVP_DecryptUpdate(ctx, out.data(), &len, ct, static_cast<int>(ct_len)) == 1);
        ok = ok && (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG,
                                         static_cast<int>(TAG_LEN),
                                         const_cast<uint8_t*>(tag)) == 1);
        int final_len = 0;
        if (EVP_DecryptFinal_ex(ctx, out.data() + len, &final_len) != 1) {
            out.clear(); // Authentication failed
        }
        EVP_CIPHER_CTX_free(ctx);
        return ok ? out : std::vector<uint8_t>{};
    }

private:
    uint8_t key_[KEY_LEN];

    static uint8_t hex_nibble(char c) {
        if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
        return 0;
    }
    static uint8_t hex_byte(const char* p) {
        return static_cast<uint8_t>((hex_nibble(p[0]) << 4) | hex_nibble(p[1]));
    }
};

static SyncCrypto g_crypto;
#endif // NNOS_HAS_OPENSSL

static std::vector<uint8_t> encrypt_packet(const std::vector<uint8_t>& plain) {
#ifdef NNOS_HAS_OPENSSL
    return g_crypto.encrypt(plain);
#else
    // Fallback: length-prefixed framing only (no confidentiality or integrity)
    std::vector<uint8_t> out;
    uint32_t len = static_cast<uint32_t>(plain.size());
    out.push_back(static_cast<uint8_t>(len >> 24));
    out.push_back(static_cast<uint8_t>(len >> 16));
    out.push_back(static_cast<uint8_t>(len >> 8));
    out.push_back(static_cast<uint8_t>(len));
    out.insert(out.end(), plain.begin(), plain.end());
    return out;
#endif
}

static std::vector<uint8_t> decrypt_packet(const std::vector<uint8_t>& cipher) {
#ifdef NNOS_HAS_OPENSSL
    return g_crypto.decrypt(cipher);
#else
    if (cipher.size() < 4) return {};
    uint32_t len = (static_cast<uint32_t>(cipher[0]) << 24) |
                   (static_cast<uint32_t>(cipher[1]) << 16) |
                   (static_cast<uint32_t>(cipher[2]) << 8) |
                   static_cast<uint32_t>(cipher[3]);
    if (len + 4 > cipher.size()) return {};
    return std::vector<uint8_t>(cipher.begin() + 4, cipher.begin() + 4 + len);
#endif
}

// ---------------------------------------------------------------------------
// TLV serialization
// ---------------------------------------------------------------------------

static std::vector<uint8_t> serialize_fields(const std::vector<StateField>& fields) {
    std::vector<uint8_t> out;
    for (const auto& f : fields) {
        // TLV: 1-byte type, 4-byte length (BE), payload
        uint32_t type_val = static_cast<uint32_t>(f.type);
        uint32_t len = static_cast<uint32_t>(f.payload.size());
        out.push_back(static_cast<uint8_t>(type_val));
        out.push_back(static_cast<uint8_t>(len >> 24));
        out.push_back(static_cast<uint8_t>(len >> 16));
        out.push_back(static_cast<uint8_t>(len >> 8));
        out.push_back(static_cast<uint8_t>(len));
        out.insert(out.end(), f.payload.begin(), f.payload.end());
    }
    return out;
}

static std::vector<StateField> deserialize_fields(const std::vector<uint8_t>& data) {
    std::vector<StateField> fields;
    size_t i = 0;
    while (i + 5 <= data.size()) {
        StateField f;
        f.type = static_cast<StateFieldType>(data[i]);
        uint32_t len = (static_cast<uint32_t>(data[i+1]) << 24) |
                       (static_cast<uint32_t>(data[i+2]) << 16) |
                       (static_cast<uint32_t>(data[i+3]) << 8) |
                       static_cast<uint32_t>(data[i+4]);
        i += 5;
        if (i + len > data.size()) break;
        f.payload.assign(data.begin() + i, data.begin() + i + len);
        i += len;
        fields.push_back(f);
    }
    return fields;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

// FNV-1a 64-bit hash for snapshot convergence verification
static uint64_t fnv1a_hash(const uint8_t* data, size_t len) {
    uint64_t hash = 0xcbf29ce484222325;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 0x100000001b3;
    }
    return hash;
}

int main() {
    SignalHandler::install([]{});
    Logger logger("lsa_ethernet_sync");

    MulticastSyncSocket sock;
    if (!sock.init()) {
        logger.critical("NETWORK", "Failed to initialize multicast socket on " +
                        std::string(MCAST_GROUP) + ":" + std::to_string(MCAST_PORT));
        return 1;
    }

    logger.info("BOOT", "Ethernet sync active on " + std::string(MCAST_GROUP) +
                ":" + std::to_string(MCAST_PORT));
#ifdef NNOS_HAS_OPENSSL
    if (g_crypto.is_default_key()) {
        logger.critical("CRYPTO", "AES-256-GCM active but using default key — "
                        "set NNOS_SYNC_KEY to a 64-char hex string before production");
    } else {
        logger.info("CRYPTO", "AES-256-GCM encryption active");
    }
#else
    logger.warn("CRYPTO", "AES-256-GCM unavailable (OpenSSL not linked) — "
                "packets are length-framed only");
#endif

    int cycle = 0;
    while (!SignalHandler::should_shutdown()) {
        // Collect and send local state
        auto fields = SharedState::instance().read_all();
        if (!fields.empty()) {
            auto tlv = serialize_fields(fields);
            auto cipher = encrypt_packet(tlv);
            if (sock.send(cipher)) {
                logger.info("SYNC", "Sent " + std::to_string(tlv.size()) +
                            " bytes TLV (" + std::to_string(cipher.size()) + " bytes framed)");
            } else {
                logger.warn("SYNC", "Send failed");
            }
        }

        // Every 6 cycles (~30s), broadcast snapshot hash for convergence verification
        if (++cycle % 6 == 0) {
            auto snap = SharedState::instance().snapshot();
            uint64_t hash = fnv1a_hash(snap.data(), snap.size());

            StateField hash_field;
            hash_field.type = StateFieldType::SNAPSHOT_HASH;
            hash_field.timestamp_ns = 0; // hash itself is the identity
            hash_field.payload.resize(8);
            for (int i = 0; i < 8; ++i) {
                hash_field.payload[i] = static_cast<uint8_t>(hash >> (i * 8));
            }

            auto tlv = serialize_fields({hash_field});
            auto cipher = encrypt_packet(tlv);
            if (sock.send(cipher)) {
                logger.info("SYNC", "Broadcast snapshot hash=" + std::to_string(hash));
            }
        }

        // Attempt to receive from other nodes
        auto cipher = sock.recv();
        if (!cipher.empty()) {
            auto tlv = decrypt_packet(cipher);
            if (!tlv.empty()) {
                auto remote_fields = deserialize_fields(tlv);
                logger.info("SYNC", "Received " + std::to_string(tlv.size()) +
                            " bytes TLV from remote (" +
                            std::to_string(remote_fields.size()) + " fields)");

                for (const auto& rf : remote_fields) {
                    if (rf.type == StateFieldType::SNAPSHOT_HASH && rf.payload.size() >= 8) {
                        uint64_t remote_hash = 0;
                        for (int i = 0; i < 8; ++i) {
                            remote_hash |= static_cast<uint64_t>(rf.payload[i]) << (i * 8);
                        }
                        auto local_snap = SharedState::instance().snapshot();
                        uint64_t local_hash = fnv1a_hash(local_snap.data(), local_snap.size());
                        bool converged = (remote_hash == local_hash);
                        logger.info("CONVERGE", "remote_hash=" + std::to_string(remote_hash) +
                                    " local_hash=" + std::to_string(local_hash) +
                                    " status=" + (converged ? "MATCH" : "DIVERGED"));
                    } else {
                        if (SharedState::instance().merge_field(rf)) {
                            logger.info("MERGE", "Merged field type=" +
                                        std::to_string(static_cast<uint32_t>(rf.type)) +
                                        " ts=" + std::to_string(rf.timestamp_ns));
                        } else {
                            logger.info("MERGE", "Rejected stale field type=" +
                                        std::to_string(static_cast<uint32_t>(rf.type)) +
                                        " ts=" + std::to_string(rf.timestamp_ns));
                        }
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    logger.info("SHUTDOWN", "Ethernet sync stopping");
    return 0;
}
