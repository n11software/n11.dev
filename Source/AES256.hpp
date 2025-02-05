#pragma once
#include <string>
#include <vector>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

class AES256 {
private:
    static constexpr int KEY_SIZE = 32;
    static constexpr int IV_SIZE = 16;
    static constexpr int SALT_SIZE = 8;
    static constexpr unsigned char SALTED_PREFIX[] = "Salted__";

    static void EVP_BytesToKey_OpenSSL(const std::string& password,
                                     const unsigned char* salt,
                                     unsigned char* key,
                                     unsigned char* iv) {
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        int nkey = KEY_SIZE;
        int niv = IV_SIZE;
        int addmd = 0;
        unsigned char key_iv[EVP_MAX_MD_SIZE];
        
        for (;;) {
            EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
            if (addmd++)
                EVP_DigestUpdate(ctx, key_iv, EVP_MD_size(EVP_md5()));
            EVP_DigestUpdate(ctx, password.c_str(), password.length());
            if (salt)
                EVP_DigestUpdate(ctx, salt, SALT_SIZE);
            EVP_DigestFinal_ex(ctx, key_iv, nullptr);
            
            int todo;
            if (nkey) {
                todo = (nkey > EVP_MD_size(EVP_md5())) ? EVP_MD_size(EVP_md5()) : nkey;
                memcpy(key, key_iv, todo);
                key += todo;
                nkey -= todo;
            }
            if (niv && !nkey) {
                todo = (niv > EVP_MD_size(EVP_md5())) ? EVP_MD_size(EVP_md5()) : niv;
                memcpy(iv, key_iv, todo);
                iv += todo;
                niv -= todo;
            }
            if (!nkey && !niv)
                break;
        }
        EVP_MD_CTX_free(ctx);
    }

public:
    static std::vector<unsigned char> base64_decode(const std::string& encoded) {
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        
        BIO* bmem = BIO_new_mem_buf(encoded.c_str(), encoded.length());
        bmem = BIO_push(b64, bmem);
        
        std::vector<unsigned char> buffer(encoded.length());
        int decoded_length = BIO_read(bmem, buffer.data(), encoded.length());
        BIO_free_all(bmem);
        
        buffer.resize(decoded_length);
        return buffer;
    }
    static std::string base64_encode(const std::vector<unsigned char>& binary) {
        BIO* bmem = BIO_new(BIO_s_mem());
        BIO* b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        BIO_push(b64, bmem);
        
        BIO_write(b64, binary.data(), binary.size());
        BIO_flush(b64);
        
        BUF_MEM* bptr;
        BIO_get_mem_ptr(b64, &bptr);
        std::string result(bptr->data, bptr->length);
        BIO_free_all(b64);
        
        return result;
    }

    static std::string encrypt(const std::string& plaintext, const std::string& password) {
        unsigned char salt[SALT_SIZE];
        RAND_bytes(salt, SALT_SIZE);
        
        unsigned char key[KEY_SIZE];
        unsigned char iv[IV_SIZE];
        EVP_BytesToKey_OpenSSL(password, salt, key, iv);
        
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);
        
        std::vector<unsigned char> ciphertext(plaintext.length() + EVP_MAX_BLOCK_LENGTH);
        int outlen1, outlen2;
        
        EVP_EncryptUpdate(ctx, ciphertext.data(), &outlen1,
                         (unsigned char*)plaintext.c_str(), plaintext.length());
        EVP_EncryptFinal_ex(ctx, ciphertext.data() + outlen1, &outlen2);
        EVP_CIPHER_CTX_free(ctx);
        
        std::vector<unsigned char> result;
        result.insert(result.end(), SALTED_PREFIX, SALTED_PREFIX + 8);
        result.insert(result.end(), salt, salt + SALT_SIZE);
        result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + outlen1 + outlen2);
        
        return base64_encode(result);
    }

    static std::string decrypt(const std::string& base64_ciphertext, const std::string& password) {
        try {
            std::vector<unsigned char> input = base64_decode(base64_ciphertext);
            if (input.size() < 16 || memcmp(input.data(), SALTED_PREFIX, 8) != 0)
                return "";

            unsigned char key[KEY_SIZE];
            unsigned char iv[IV_SIZE];
            EVP_BytesToKey_OpenSSL(password, input.data() + 8, key, iv);

            EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
            EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv);

            std::vector<unsigned char> plaintext(input.size());
            int outlen1, outlen2;

            EVP_DecryptUpdate(ctx, plaintext.data(), &outlen1,
                            input.data() + 16, input.size() - 16);
            EVP_DecryptFinal_ex(ctx, plaintext.data() + outlen1, &outlen2);
            EVP_CIPHER_CTX_free(ctx);

            return std::string((char*)plaintext.data(), outlen1 + outlen2);
        } catch (...) {
            return "";
        }
    }
};
