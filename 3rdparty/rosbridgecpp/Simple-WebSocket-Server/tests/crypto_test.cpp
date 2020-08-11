#include "assert.hpp"
#include "crypto.hpp"
#include <vector>

using namespace std;
using namespace SimpleWeb;

const vector<pair<string, string>> base64_string_tests = {
    {"", ""},
    {"f", "Zg=="},
    {"fo", "Zm8="},
    {"foo", "Zm9v"},
    {"foob", "Zm9vYg=="},
    {"fooba", "Zm9vYmE="},
    {"foobar", "Zm9vYmFy"},
    {"The itsy bitsy spider climbed up the waterspout.\r\nDown came the rain\r\nand washed the spider out.\r\nOut came the sun\r\nand dried up all the rain\r\nand the itsy bitsy spider climbed up the spout again.",
     "VGhlIGl0c3kgYml0c3kgc3BpZGVyIGNsaW1iZWQgdXAgdGhlIHdhdGVyc3BvdXQuDQpEb3duIGNhbWUgdGhlIHJhaW4NCmFuZCB3YXNoZWQgdGhlIHNwaWRlciBvdXQuDQpPdXQgY2FtZSB0aGUgc3VuDQphbmQgZHJpZWQgdXAgYWxsIHRoZSByYWluDQphbmQgdGhlIGl0c3kgYml0c3kgc3BpZGVyIGNsaW1iZWQgdXAgdGhlIHNwb3V0IGFnYWluLg=="}};

const vector<pair<string, string>> md5_string_tests = {
    {"", "d41d8cd98f00b204e9800998ecf8427e"},
    {"The quick brown fox jumps over the lazy dog", "9e107d9d372bb6826bd81d3542a419d6"}};

const vector<pair<string, string>> sha1_string_tests = {
    {"", "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
    {"The quick brown fox jumps over the lazy dog", "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12"}};

const vector<pair<string, string>> sha256_string_tests = {
    {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
    {"The quick brown fox jumps over the lazy dog", "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"}};

const vector<pair<string, string>> sha512_string_tests = {
    {"", "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"},
    {"The quick brown fox jumps over the lazy dog", "07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6"}};

int main() {
  for(auto &string_test : base64_string_tests) {
    ASSERT(Crypto::Base64::encode(string_test.first) == string_test.second);
    ASSERT(Crypto::Base64::decode(string_test.second) == string_test.first);
  }

  for(auto &string_test : md5_string_tests) {
    ASSERT(Crypto::to_hex_string(Crypto::md5(string_test.first)) == string_test.second);
    stringstream ss(string_test.first);
    ASSERT(Crypto::to_hex_string(Crypto::md5(ss)) == string_test.second);
  }

  for(auto &string_test : sha1_string_tests) {
    ASSERT(Crypto::to_hex_string(Crypto::sha1(string_test.first)) == string_test.second);
    stringstream ss(string_test.first);
    ASSERT(Crypto::to_hex_string(Crypto::sha1(ss)) == string_test.second);
  }

  for(auto &string_test : sha256_string_tests) {
    ASSERT(Crypto::to_hex_string(Crypto::sha256(string_test.first)) == string_test.second);
    stringstream ss(string_test.first);
    ASSERT(Crypto::to_hex_string(Crypto::sha256(ss)) == string_test.second);
  }

  for(auto &string_test : sha512_string_tests) {
    ASSERT(Crypto::to_hex_string(Crypto::sha512(string_test.first)) == string_test.second);
    stringstream ss(string_test.first);
    ASSERT(Crypto::to_hex_string(Crypto::sha512(ss)) == string_test.second);
  }

  //Testing iterations
  ASSERT(Crypto::to_hex_string(Crypto::sha1("Test", 1)) == "640ab2bae07bedc4c163f679a746f7ab7fb5d1fa");
  ASSERT(Crypto::to_hex_string(Crypto::sha1("Test", 2)) == "af31c6cbdecd88726d0a9b3798c71ef41f1624d5");
  stringstream ss("Test");
  ASSERT(Crypto::to_hex_string(Crypto::sha1(ss, 2)) == "af31c6cbdecd88726d0a9b3798c71ef41f1624d5");

  ASSERT(Crypto::to_hex_string(Crypto::pbkdf2("Password", "Salt", 4096, 128 / 8)) == "f66df50f8aaa11e4d9721e1312ff2e66");
  ASSERT(Crypto::to_hex_string(Crypto::pbkdf2("Password", "Salt", 8192, 512 / 8)) == "a941ccbc34d1ee8ebbd1d34824a419c3dc4eac9cbc7c36ae6c7ca8725e2b618a6ad22241e787af937b0960cf85aa8ea3a258f243e05d3cc9b08af5dd93be046c");
}
