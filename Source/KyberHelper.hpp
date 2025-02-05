#pragma once
#include <string>
#include <vector>
#include <stdexcept>

// OpenSSL headers
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

// OQS header
#include <oqs/oqs.h>

class KyberHelper {
public:
    struct KeyPair {
        std::string publicKey;
        std::string privateKey;
        std::string error;  // Add error field
        bool success() const { return error.empty(); }
    };

    struct MessagePair {
        std::string ciphertext;
        std::string sharedSecret;
        std::string error;  // Add error field
        bool success() const { return error.empty(); }
    };

    static KeyPair generateKeyPair() {
        try {
            OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_kyber_1024);
            if (!kem) return KeyPair{"", "", "Failed to initialize Kyber1024"};

            uint8_t *public_key = new uint8_t[kem->length_public_key];
            uint8_t *private_key = new uint8_t[kem->length_secret_key];

            OQS_STATUS status = OQS_KEM_keypair(kem, public_key, private_key);
            if (status != OQS_SUCCESS) {
                delete[] public_key;
                delete[] private_key;
                OQS_KEM_free(kem);
                return KeyPair{"", "", "Failed to generate keypair"};
            }

            KeyPair result = {
                base64Encode(public_key, kem->length_public_key),
                base64Encode(private_key, kem->length_secret_key),
                ""  // No error
            };

            delete[] public_key;
            delete[] private_key;
            OQS_KEM_free(kem);
            return result;
        } catch (const std::exception& e) {
            return KeyPair{"", "", std::string("Key generation failed: ") + e.what()};
        }
    }

    static MessagePair encrypt(const std::string& message, const std::string& publicKeyB64) {
        try {
            OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_kyber_1024);
            if (!kem) return MessagePair{"", "", "Failed to initialize Kyber1024"};

            std::vector<uint8_t> public_key = base64Decode(publicKeyB64);
            uint8_t *ciphertext = new uint8_t[kem->length_ciphertext];
            uint8_t *shared_secret = new uint8_t[kem->length_shared_secret];

            if (OQS_KEM_encaps(kem, ciphertext, shared_secret, public_key.data()) != OQS_SUCCESS) {
                delete[] ciphertext;
                delete[] shared_secret;
                OQS_KEM_free(kem);
                return MessagePair{"", "", "Encryption failed"};
            }

            // First encrypt the message
            std::string encrypted_message = aesEncrypt(message, shared_secret, kem->length_shared_secret);

            // Prepare result
            MessagePair result = {
                base64Encode(ciphertext, kem->length_ciphertext),
                base64Encode(reinterpret_cast<const unsigned char*>(encrypted_message.data()), 
                            encrypted_message.length()),
                ""  // No error
            };

            delete[] ciphertext;
            delete[] shared_secret;
            OQS_KEM_free(kem);
            return result;
        } catch (const std::exception& e) {
            return MessagePair{"", "", std::string("Encryption failed: ") + e.what()};
        }
    }

    static std::string decrypt(const std::string& ciphertextB64, 
                             const std::string& encryptedMessageB64,
                             const std::string& privateKeyB64) {
        try {
            OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_kyber_1024);
            if (!kem) throw std::runtime_error("Failed to initialize Kyber1024");

            // Decode inputs
            std::vector<uint8_t> ciphertext = base64Decode(ciphertextB64);
            std::vector<uint8_t> private_key = base64Decode(privateKeyB64);
            std::vector<uint8_t> encrypted_message = base64Decode(encryptedMessageB64);

            // Recover shared secret
            uint8_t *shared_secret = new uint8_t[kem->length_shared_secret];
            if (OQS_KEM_decaps(kem, shared_secret, ciphertext.data(), private_key.data()) != OQS_SUCCESS) {
                delete[] shared_secret;
                OQS_KEM_free(kem);
                throw std::runtime_error("Failed to recover shared secret");
            }

            // Decrypt the message
            std::string decrypted;
            try {
                decrypted = aesDecrypt(encrypted_message, shared_secret, kem->length_shared_secret);
            } catch (const std::exception& e) {
                delete[] shared_secret;
                OQS_KEM_free(kem);
                throw;
            }

            delete[] shared_secret;
            OQS_KEM_free(kem);
            return decrypted;
        } catch (const std::exception& e) {
            return std::string("Decryption failed: ") + e.what();
        }
    }

    static bool verifyKeyPair(const std::string& publicKeyB64, const std::string& privateKeyB64) {
        try {
            auto msgPair = encrypt("test", publicKeyB64);
            if (!msgPair.success()) {
                return false;
            }
            std::string decryptedSecret = decrypt(msgPair.ciphertext, msgPair.sharedSecret, privateKeyB64);
            return !decryptedSecret.empty() && decryptedSecret == "test";
        } catch (const std::exception&) {
            return false;
        }
    }

    // Move these from private to public
    static std::string base64Encode(const unsigned char* data, size_t length) {
        try {
            BIO* b64 = BIO_new(BIO_f_base64());
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            BIO* bmem = BIO_new(BIO_s_mem());
            b64 = BIO_push(b64, bmem);
            BIO_write(b64, data, length);
            BIO_flush(b64);
            BUF_MEM* bptr;
            BIO_get_mem_ptr(b64, &bptr);
            std::string result(bptr->data, bptr->length);
            BIO_free_all(b64);
            return result;
        } catch (const std::exception&) {
            return "";
        }
    }

    static std::vector<unsigned char> base64Decode(const std::string& input) {
        try {
            BIO* b64 = BIO_new(BIO_f_base64());
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            BIO* bmem = BIO_new_mem_buf(input.c_str(), -1);
            bmem = BIO_push(b64, bmem);
            std::vector<unsigned char> result(input.length());
            int decoded_size = BIO_read(bmem, result.data(), input.length());
            BIO_free_all(bmem);
            if (decoded_size <= 0) {
                throw std::runtime_error("Failed to decode base64 data");
            }
            result.resize(decoded_size);
            return result;
        } catch (const std::exception&) {
            return std::vector<unsigned char>();
        }
    }

private:
    static std::string aesEncrypt(const std::string& plaintext, const uint8_t* key, size_t keylen) {
        try {
            EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
            if (!ctx) return "";

            unsigned char iv[12];
            if (RAND_bytes(iv, sizeof(iv)) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                return "";
            }

            if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key, iv) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                return "";
            }

            std::vector<unsigned char> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
            int len;
            if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, 
                reinterpret_cast<const unsigned char*>(plaintext.data()), 
                plaintext.size()) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                return "";
            }

            int ciphertext_len = len;
            if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                return "";
            }
            ciphertext_len += len;

            unsigned char tag[16];
            if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                return "";
            }

            EVP_CIPHER_CTX_free(ctx);

            std::string result;
            result.reserve(12 + 16 + ciphertext_len);
            result.append(reinterpret_cast<char*>(iv), 12);
            result.append(reinterpret_cast<char*>(tag), 16);
            result.append(reinterpret_cast<char*>(ciphertext.data()), ciphertext_len);
            return result;
        } catch (const std::exception&) {
            return "";
        }
    }

    static std::string aesDecrypt(const std::vector<uint8_t>& ciphertext, const uint8_t* key, size_t keylen) {
        try {
            if (ciphertext.size() < 28) return "";

            EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
            if (!ctx) return "";

            const unsigned char* iv = ciphertext.data();
            const unsigned char* tag = ciphertext.data() + 12;
            const unsigned char* encrypted = ciphertext.data() + 28;
            size_t encrypted_len = ciphertext.size() - 28;

            if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key, iv) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                return "";
            }

            std::vector<unsigned char> plaintext(encrypted_len + EVP_MAX_BLOCK_LENGTH);
            int len;
            if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, encrypted, encrypted_len) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                return "";
            }

            if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, (void*)tag) != 1) {
                EVP_CIPHER_CTX_free(ctx);
                return "";
            }

            int plaintext_len = len;
            int ret = EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
            EVP_CIPHER_CTX_free(ctx);

            if (ret <= 0) return "";
            plaintext_len += len;

            return std::string(reinterpret_cast<char*>(plaintext.data()), plaintext_len);
        } catch (const std::exception&) {
            return "";
        }
    }
};
