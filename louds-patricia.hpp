#ifndef LOUDS_PATRICIA_HPP
#define LOUDS_PATRICIA_HPP

#include "bit-vector.hpp"
#include "trie-base.hpp"

namespace trie_eval {

using namespace std;

class LoudsPatricia : TrieBase {
 public:
  LoudsPatricia();
  ~LoudsPatricia() {}

  void build(const vector<string> &keys);

  uint64_t lookup(const string &query) const;
  void reverse_lookup(uint64_t id, string &key) const;

  const char *name() const {
    return "LoudsPatricia";
  }
  uint64_t n_keys() const {
    return n_keys_;
  }
  uint64_t n_nodes() const {
    return n_nodes_;
  }
  uint64_t size() const {
    return size_;
  }

 private:
  BitVector louds_;
  BitVector outs_;
  BitVector links_;
  vector<uint8_t> labels_;
  BitVector tail_bits_;
  vector<uint8_t> tail_bytes_;
  uint64_t n_keys_;
  uint64_t n_nodes_;
  uint64_t size_;
};

}  // namespace trie_eval

#endif  // LOUDS_PATRICIA_HPP
