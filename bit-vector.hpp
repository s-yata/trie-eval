#ifndef BIT_VECTOR_HPP
#define BIT_VECTOR_HPP

#include <x86intrin.h>

#include <cassert>
#include <cstdint>
#include <vector>

namespace trie_eval {

using namespace std;

struct BitVector {
  vector<uint64_t> words;
  uint64_t n_bits;
  struct Rank {
    uint32_t abs_hi;
    uint8_t abs_lo;
    uint8_t rels[3];

    uint64_t abs() const {
      return ((uint64_t)abs_hi << 8) | abs_lo;
    }
    void set_abs(uint64_t abs) {
      abs_hi = (uint32_t)(abs >> 8);
      abs_lo = (uint8_t)abs;
    }
  };
  vector<Rank> ranks;
  vector<uint64_t> select0s;
  vector<uint64_t> select1s;
  uint64_t n_zeros;
  uint64_t n_ones;

  BitVector()
    : words(), n_bits(0), ranks(), select0s(), select1s(),
      n_zeros(0), n_ones(0) {}
  ~BitVector() {}

  uint64_t size() const {
    return (sizeof(uint64_t) * words.size())
      + (sizeof(Rank) * ranks.size())
      + (sizeof(uint64_t) * select0s.size())
      + (sizeof(uint64_t) * select1s.size());
  }

  uint64_t operator[](uint64_t i) const {
    assert(i < n_bits);
    return (words[i / 64] >> (i % 64)) & 1;
  }
  void set(uint64_t i, uint64_t bit) {
    assert(i < n_bits);
    if (bit) {
      words[i / 64] |= (1UL << (i % 64));
    } else {
      words[i / 64] &= ~(1UL << (i % 64));
    }
  }

  void add(uint64_t bit) {
    if (n_bits % 256 == 0) {
      words.resize((n_bits + 256) / 64, 0);
    }
    if (bit) {
      words[n_bits / 64] |= (1UL << (n_bits % 64));
    }
    ++n_bits;
  }

  void build() {
    uint64_t n_blocks = words.size() / 4;
    ranks.resize(n_blocks + 1);
    n_zeros = 0;
    n_ones = 0;
    for (uint64_t block_id = 0; block_id < n_blocks; ++block_id) {
      ranks[block_id].set_abs(n_ones);
      for (uint64_t j = 0; j < 4; ++j) {
        if (j != 0) {
          uint64_t rel = n_ones - ranks[block_id].abs();
          ranks[block_id].rels[j - 1] = rel;
        }

        uint64_t word_id = (block_id * 4) + j;
        uint64_t n_pops = __builtin_popcountll(words[word_id]);
        uint64_t new_n_zeros = n_zeros + 64 - n_pops;
        if (((n_zeros + 255) / 256) != ((new_n_zeros + 255) / 256)) {
          uint64_t count = n_zeros;
          uint64_t word = ~words[word_id];
          while (word != 0) {
            uint64_t pos = __builtin_ctzll(word);
            if (count % 256 == 0) {
              select0s.push_back(((word_id * 64) + pos) >> 8);
              break;
            }
            word ^= 1UL << pos;
            ++count;
          }
        }
        n_zeros = new_n_zeros;

        uint64_t new_n_ones = n_ones + n_pops;
        if (((n_ones + 255) / 256) != ((new_n_ones + 255) / 256)) {
          uint64_t count = n_ones;
          uint64_t word = words[word_id];
          while (word != 0) {
            uint64_t pos = __builtin_ctzll(word);
            if (count % 256 == 0) {
              select1s.push_back(((word_id * 64) + pos) >> 8);
              break;
            }
            word ^= 1UL << pos;
            ++count;
          }
        }
        n_ones = new_n_ones;
      }
    }
    ranks.back().set_abs(n_ones);
    select0s.push_back(words.size() * 64 / 256);
    select1s.push_back(words.size() * 64 / 256);
  }

  uint64_t rank0(uint64_t i) const {
    return i - rank1(i);
  }
  uint64_t rank1(uint64_t i) const {
    uint64_t word_id = i / 64;
    uint64_t bit_id = i % 64;
    uint64_t rank_id = word_id / 4;
    uint64_t rel_id = word_id % 4;
    uint64_t n = ranks[rank_id].abs();
    if (rel_id != 0) {
      n += ranks[rank_id].rels[rel_id - 1];
    }
    n += __builtin_popcountll(words[word_id] & ((1UL << bit_id) - 1));
    return n;
  }

  uint64_t select0(uint64_t i) const {
    const uint64_t block_id = i / 256;
    uint64_t begin = select0s[block_id];
    uint64_t end = select0s[block_id + 1] + 1;
    if (begin + 10 >= end) {
      while (i >= (256 * (begin + 1)) - ranks[begin + 1].abs()) {
        ++begin;
      }
    } else {
      while (begin + 1 < end) {
        const uint64_t middle = (begin + end) / 2;
        if (i < (256 * middle) - ranks[middle].abs()) {
          end = middle;
        } else {
          begin = middle;
        }
      }
    }
    const uint64_t rank_id = begin;
    i -= (256 * rank_id) - ranks[rank_id].abs();

    uint64_t word_id = rank_id * 4;
    if (i < 128UL - ranks[rank_id].rels[1]) {
      if (i >= 64UL - ranks[rank_id].rels[0]) {
        word_id += 1;
        i -= 64UL - ranks[rank_id].rels[0];
      }
    } else if (i < 192UL - ranks[rank_id].rels[2]) {
      word_id += 2;
      i -= 128UL - ranks[rank_id].rels[1];
    } else {
      word_id += 3;
      i -= 192UL - ranks[rank_id].rels[2];
    }
    return (word_id * 64) + __builtin_ctzll(
      _pdep_u64(1UL << i, ~words[word_id]));
  }
  uint64_t select1(uint64_t i) const {
    const uint64_t block_id = i / 256;
    uint64_t begin = select1s[block_id];
    uint64_t end = select1s[block_id + 1] + 1;
    if (begin + 10 >= end) {
      while (i >= ranks[begin + 1].abs()) {
        ++begin;
      }
    } else {
      while (begin + 1 < end) {
        const uint64_t middle = (begin + end) / 2;
        if (i < ranks[middle].abs()) {
          end = middle;
        } else {
          begin = middle;
        }
      }
    }
    const uint64_t rank_id = begin;
    i -= ranks[rank_id].abs();

    uint64_t word_id = rank_id * 4;
    if (i < ranks[rank_id].rels[1]) {
      if (i >= ranks[rank_id].rels[0]) {
        word_id += 1;
        i -= ranks[rank_id].rels[0];
      }
    } else if (i < ranks[rank_id].rels[2]) {
      word_id += 2;
      i -= ranks[rank_id].rels[1];
    } else {
      word_id += 3;
      i -= ranks[rank_id].rels[2];
    }
    return (word_id * 64) + __builtin_ctzll(
      _pdep_u64(1UL << i, words[word_id]));
  }
};

}  // namespace trie_eval

#endif  // BIT_VECTOR_HPP
