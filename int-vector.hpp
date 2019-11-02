#ifndef INT_VECTOR_HPP
#define INT_VECTOR_HPP

#include <cassert>
#include <cstdint>
#include <vector>

namespace trie_eval {

using namespace std;

struct IntVector {
  vector<uint64_t> words;
  uint64_t n_ints;
  uint64_t n_bits;
  uint64_t mask;

  IntVector() : words(), n_ints(0), n_bits(0) {}
  ~IntVector() {}

  void init(uint64_t n, uint64_t max_value) {
    words.clear();
    n_ints = n;
    n_bits = (max_value != 0) ? (64 - __builtin_clzll(max_value)) : 1;
    mask = (1UL << n_bits) - 1;
    words.resize((n * n_bits + 63) / 64, 0);
  }

  uint64_t size() const {
    return sizeof(uint64_t) * words.size();
  }

  uint64_t operator[](uint64_t i) const {
    assert(i < n_ints);
    const std::size_t pos = i * n_bits;
    const std::size_t id = pos / 64;
    const std::size_t offset = pos % 64;

    if ((offset + n_bits) <= 64) {
      return (words[id] >> offset) & mask;
    } else {
      return ((words[id] >> offset) | (words[id + 1] << (64 - offset))) & mask;
    }
  }
  void set(uint64_t i, uint64_t value) {
    assert(i < n_ints);
    const std::size_t pos = i * n_bits;
    const std::size_t id = pos / 64;
    const std::size_t offset = pos % 64;

    words[id] &= ~(mask << offset);
    words[id] |= (value & mask) << offset;
    if ((offset + n_bits) > 64) {
      words[id + 1] &= ~(mask >> (64 - offset));
      words[id + 1] |= (value & mask) >> (64 - offset);
    }
  }

  void add(uint64_t value) {
    const std::size_t pos = n_ints * n_bits;
    const std::size_t id = pos / 64;
    const std::size_t offset = pos % 64;

    if ((pos / 64) != ((pos + n_bits) / 64)) {
      words.push_back(0);
    }
    words[id] &= ~(mask << offset);
    words[id] |= (value & mask) << offset;
    if ((offset + n_bits) > 64) {
      words[id + 1] &= ~(mask >> (64 - offset));
      words[id + 1] |= (value & mask) >> (64 - offset);
    }
    ++n_ints;
  }
};

}  // namespace trie_evalue

#endif  // INT_VECTOR_HPP
