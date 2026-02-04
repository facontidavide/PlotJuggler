#include <array>
#include <cstddef>
#include <cstdint>

namespace mcap::internal {

/**
 * Compute CRC32 lookup tables as described at:
 * https://github.com/komrad36/CRC#option-6-1-byte-tabular
 *
 * An iteration of CRC computation can be performed on 8 bits of input at once. By pre-computing a
 * table of the values of CRC(?) for all 2^8 = 256 possible byte values, during the final
 * computation we can replace a loop over 8 bits with a single lookup in the table.
 *
 * For further speedup, we can also pre-compute the values of CRC(?0) for all possible bytes when a
 * zero byte is appended. Then we can process two bytes of input at once by computing CRC(AB) =
 * CRC(A0) ^ CRC(B), using one lookup in the CRC(?0) table and one lookup in the CRC(?) table.
 *
 * The same technique applies for any number of bytes to be processed at once, although the speed
 * improvements diminish.
 *
 * @param Polynomial The binary representation of the polynomial to use (reversed, i.e. most
 * significant bit represents x^0).
 * @param NumTables The number of bytes of input that will be processed at once.
 */
template <size_t Polynomial, size_t NumTables>
struct CRC32Table {
private:
  std::array<uint32_t, 256 * NumTables> table = {};

public:
  constexpr CRC32Table() {
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t r = i;
      r = ((r & 1) * Polynomial) ^ (r >> 1);
      r = ((r & 1) * Polynomial) ^ (r >> 1);
      r = ((r & 1) * Polynomial) ^ (r >> 1);
      r = ((r & 1) * Polynomial) ^ (r >> 1);
      r = ((r & 1) * Polynomial) ^ (r >> 1);
      r = ((r & 1) * Polynomial) ^ (r >> 1);
      r = ((r & 1) * Polynomial) ^ (r >> 1);
      r = ((r & 1) * Polynomial) ^ (r >> 1);
      table[i] = r;
    }
    for (size_t i = 256; i < table.size(); i++) {
      uint32_t value = table[i - 256];
      table[i] = table[value & 0xff] ^ (value >> 8);
    }
  }

  constexpr uint32_t operator[](size_t index) const {
    return table[index];
  }
};

inline uint32_t getUint32LE(const std::byte* data) {
  return (uint32_t(data[0]) << 0) | (uint32_t(data[1]) << 8) | (uint32_t(data[2]) << 16) |
         (uint32_t(data[3]) << 24);
}

static constexpr CRC32Table<0xedb88320, 8> CRC32_TABLE;

/**
 * Initialize a CRC32 to all 1 bits.
 */
static constexpr uint32_t CRC32_INIT = 0xffffffff;

/**
 * Update a streaming CRC32 calculation.
 *
 * For performance, this implementation processes the data 8 bytes at a time, using the algorithm
 * presented at: https://github.com/komrad36/CRC#option-9-8-byte-tabular
 */
inline uint32_t crc32Update(const uint32_t prev, const std::byte* const data, const size_t length) {
  // Process bytes one by one until we reach the proper alignment.
  uint32_t r = prev;
  size_t offset = 0;
  for (; (uintptr_t(data + offset) & alignof(uint32_t)) != 0 && offset < length; offset++) {
    r = CRC32_TABLE[(r ^ uint8_t(data[offset])) & 0xff] ^ (r >> 8);
  }
  if (offset == length) {
    return r;
  }

  // Process 8 bytes (2 uint32s) at a time.
  size_t remainingBytes = length - offset;
  for (; remainingBytes >= 8; offset += 8, remainingBytes -= 8) {
    r ^= getUint32LE(data + offset);
    uint32_t r2 = getUint32LE(data + offset + 4);
    r = CRC32_TABLE[0 * 256 + ((r2 >> 24) & 0xff)] ^ CRC32_TABLE[1 * 256 + ((r2 >> 16) & 0xff)] ^
        CRC32_TABLE[2 * 256 + ((r2 >> 8) & 0xff)] ^ CRC32_TABLE[3 * 256 + ((r2 >> 0) & 0xff)] ^
        CRC32_TABLE[4 * 256 + ((r >> 24) & 0xff)] ^ CRC32_TABLE[5 * 256 + ((r >> 16) & 0xff)] ^
        CRC32_TABLE[6 * 256 + ((r >> 8) & 0xff)] ^ CRC32_TABLE[7 * 256 + ((r >> 0) & 0xff)];
  }

  // Process any remaining bytes one by one.
  for (; offset < length; offset++) {
    r = CRC32_TABLE[(r ^ uint8_t(data[offset])) & 0xff] ^ (r >> 8);
  }
  return r;
}

/** Finalize a CRC32 by inverting the output value. */
inline uint32_t crc32Final(uint32_t crc) {
  return crc ^ 0xffffffff;
}

}  // namespace mcap::internal
