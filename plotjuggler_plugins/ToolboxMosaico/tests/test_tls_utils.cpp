#include <gtest/gtest.h>
#include "src/ui/security/tls_utils.h"

TEST(TlsUtils, ValidApiKey)
{
  EXPECT_TRUE(isValidApiKey(
      "msco_s3l8gcdwuadege3pkhou0k0n2t5omfij_f9010b9e"));
}

TEST(TlsUtils, ApiKeyWrongPrefix)
{
  EXPECT_FALSE(isValidApiKey(
      "xxxx_s3l8gcdwuadege3pkhou0k0n2t5omfij_f9010b9e"));
}

TEST(TlsUtils, ApiKeyTooShort)
{
  EXPECT_FALSE(isValidApiKey("msco_abc_12345678"));
}

TEST(TlsUtils, ApiKeyEmpty)
{
  EXPECT_FALSE(isValidApiKey(""));
}

TEST(TlsUtils, ApiKeyBadFingerprint)
{
  EXPECT_FALSE(isValidApiKey(
      "msco_s3l8gcdwuadege3pkhou0k0n2t5omfij_zzzzzzzz"));
}
