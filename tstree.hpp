#ifndef TSTREE_HPP
#define TSTREE_HPP

#include "bit-vector.hpp"
#include "trie-base.hpp"

namespace trie_eval {

using namespace std;

class TSTree : TrieBase {
 public:
  TSTree();
  ~TSTree() {}

  void build(const vector<string> &keys);

  uint64_t lookup(const string &query) const;
  void reverse_lookup(uint64_t id, string &key) const;

  const char *name() const {
    return "Ternary search tree + labels";
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
  BitVector tree_;
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

#endif  // TSTREE_HPP
