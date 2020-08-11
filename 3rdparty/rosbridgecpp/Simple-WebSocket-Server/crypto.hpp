#ifndef SIMPLE_WEB_CRYPTO_HPP
#define SIMPLE_WEB_CRYPTO_HPP

#include <cmath>
#include <iomanip>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

namespace SimpleWeb {
// TODO 2017: remove workaround for MSVS 2012
#if _MSC_VER == 1700                       // MSVS 2012 has no definition for round()
  inline double round(double x) noexcept { // Custom definition of round() for positive numbers
    return floor(x + 0.5);
  }
#endif

  class Crypto {
    const static std::size_t buffer_size = 131072;

  public:
    class Base64 {
    public:
      static std::string encode(const std::string &ascii) noexcept {
        std::string base64;

        BIO *bio, *b64;
        BUF_MEM *bptr = BUF_MEM_new();

        b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
        bio = BIO_new(BIO_s_mem());
        BIO_push(b64, bio);
        BIO_set_mem_buf(b64, bptr, BIO_CLOSE);

        // Write directly to base64-buffer to avoid copy
        auto base64_length = static_cast<std::size_t>(round(4 * ceil(static_cast<double>(ascii.size()) / 3.0)));
        base64.resize(base64_length);
        bptr->length = 0;
        bptr->max = base64_length + 1;
        bptr->data = &base64[0];

        if(BIO_write(b64, &ascii[0], static_cast<int>(ascii.size())) <= 0 || BIO_flush(b64) <= 0)
          base64.clear();

        // To keep &base64[0] through BIO_free_all(b64)
        bptr->length = 0;
        bptr->max = 0;
        bptr->data = nullptr;

        BIO_free_all(b64);

        return base64;
      }

      static std::string decode(const std::string &base64) noexcept {
        std::string ascii;

        // Resize ascii, however, the size is a up to two bytes too large.
        ascii.resize((6 * base64.size()) / 8);
        BIO *b64, *bio;

        b64 = BIO_new(BIO_f_base64());
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
// TODO: Remove in 2020
#if(defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER <= 0x1000115fL) || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x2080000fL)
        bio = BIO_new_mem_buf((char *)&base64[0], static_cast<int>(base64.size()));
#else
        bio = BIO_new_mem_buf(&base64[0], static_cast<int>(base64.size()));
#endif
        bio = BIO_push(b64, bio);

        auto decoded_length = BIO_read(bio, &ascii[0], static_cast<int>(ascii.size()));
        if(decoded_length > 0)
          ascii.resize(static_cast<std::size_t>(decoded_length));
        else
          ascii.clear();

        BIO_free_all(b64);

        return ascii;
      }
    };

    /// Return hex string from bytes in input string.
    static std::string to_hex_string(const std::string &input) noexcept {
      std::stringstream hex_stream;
      hex_stream << std::hex << std::internal << std::setfill('0');
      for(auto &byte : input)
        hex_stream << std::setw(2) << static_cast<int>(static_cast<unsigned char>(byte));
      return hex_stream.str();
    }

    static std::string md5(const std::string &input, std::size_t iterations = 1) noexcept {
      std::string hash;

      hash.resize(128 / 8);
      MD5(reinterpret_cast<const unsigned char *>(&input[0]), input.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      for(std::size_t c = 1; c < iterations; ++c)
        MD5(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      return hash;
    }

    static std::string md5(std::istream &stream, std::size_t iterations = 1) noexcept {
      MD5_CTX context;
      MD5_Init(&context);
      std::streamsize read_length;
      std::vector<char> buffer(buffer_size);
      while((read_length = stream.read(&buffer[0], buffer_size).gcount()) > 0)
        MD5_Update(&context, buffer.data(), static_cast<std::size_t>(read_length));
      std::string hash;
      hash.resize(128 / 8);
      MD5_Final(reinterpret_cast<unsigned char *>(&hash[0]), &context);

      for(std::size_t c = 1; c < iterations; ++c)
        MD5(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      return hash;
    }

    static std::string sha1(const std::string &input, std::size_t iterations = 1) noexcept {
      std::string hash;

      hash.resize(160 / 8);
      SHA1(reinterpret_cast<const unsigned char *>(&input[0]), input.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      for(std::size_t c = 1; c < iterations; ++c)
        SHA1(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      return hash;
    }

    static std::string sha1(std::istream &stream, std::size_t iterations = 1) noexcept {
      SHA_CTX context;
      SHA1_Init(&context);
      std::streamsize read_length;
      std::vector<char> buffer(buffer_size);
      while((read_length = stream.read(&buffer[0], buffer_size).gcount()) > 0)
        SHA1_Update(&context, buffer.data(), static_cast<std::size_t>(read_length));
      std::string hash;
      hash.resize(160 / 8);
      SHA1_Final(reinterpret_cast<unsigned char *>(&hash[0]), &context);

      for(std::size_t c = 1; c < iterations; ++c)
        SHA1(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      return hash;
    }

    static std::string sha256(const std::string &input, std::size_t iterations = 1) noexcept {
      std::string hash;

      hash.resize(256 / 8);
      SHA256(reinterpret_cast<const unsigned char *>(&input[0]), input.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      for(std::size_t c = 1; c < iterations; ++c)
        SHA256(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      return hash;
    }

    static std::string sha256(std::istream &stream, std::size_t iterations = 1) noexcept {
      SHA256_CTX context;
      SHA256_Init(&context);
      std::streamsize read_length;
      std::vector<char> buffer(buffer_size);
      while((read_length = stream.read(&buffer[0], buffer_size).gcount()) > 0)
        SHA256_Update(&context, buffer.data(), static_cast<std::size_t>(read_length));
      std::string hash;
      hash.resize(256 / 8);
      SHA256_Final(reinterpret_cast<unsigned char *>(&hash[0]), &context);

      for(std::size_t c = 1; c < iterations; ++c)
        SHA256(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      return hash;
    }

    static std::string sha512(const std::string &input, std::size_t iterations = 1) noexcept {
      std::string hash;

      hash.resize(512 / 8);
      SHA512(reinterpret_cast<const unsigned char *>(&input[0]), input.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      for(std::size_t c = 1; c < iterations; ++c)
        SHA512(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      return hash;
    }

    static std::string sha512(std::istream &stream, std::size_t iterations = 1) noexcept {
      SHA512_CTX context;
      SHA512_Init(&context);
      std::streamsize read_length;
      std::vector<char> buffer(buffer_size);
      while((read_length = stream.read(&buffer[0], buffer_size).gcount()) > 0)
        SHA512_Update(&context, buffer.data(), static_cast<std::size_t>(read_length));
      std::string hash;
      hash.resize(512 / 8);
      SHA512_Final(reinterpret_cast<unsigned char *>(&hash[0]), &context);

      for(std::size_t c = 1; c < iterations; ++c)
        SHA512(reinterpret_cast<const unsigned char *>(&hash[0]), hash.size(), reinterpret_cast<unsigned char *>(&hash[0]));

      return hash;
    }

    /// key_size is number of bytes of the returned key.
    static std::string pbkdf2(const std::string &password, const std::string &salt, int iterations, int key_size) noexcept {
      std::string key;
      key.resize(static_cast<std::size_t>(key_size));
      PKCS5_PBKDF2_HMAC_SHA1(password.c_str(), password.size(),
                             reinterpret_cast<const unsigned char *>(salt.c_str()), salt.size(), iterations,
                             key_size, reinterpret_cast<unsigned char *>(&key[0]));
      return key;
    }
  };
} // namespace SimpleWeb
#endif /* SIMPLE_WEB_CRYPTO_HPP */
