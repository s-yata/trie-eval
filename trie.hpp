#ifndef TRIE_HPP
#define TRIE_HPP

#include "bit-vector.hpp"
#include "trie-base.hpp"

namespace trie_eval {

using namespace std;

class Trie : TrieBase {
 public:
  Trie();
  ~Trie() {}

  void build(const vector<string> &keys);

  uint64_t lookup(const string &query) const;
  void reverse_lookup(uint64_t id, string &key) const;

  const char *name() const {
    return "LOUDS trie";
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
  struct Level {
    BitVector louds;
    BitVector outs;
    vector<uint8_t> labels;
    uint64_t offset;

    Level() : louds(), outs(), labels(), offset(0) {}

    uint64_t size() const {
      return louds.size() + outs.size() + labels.size();
    }
  };

  vector<Level> levels_;
  uint64_t n_keys_;
  uint64_t n_nodes_;
  uint64_t size_;
  string last_key_;

  void add(const string &key);
};

}  // namespace trie_eval

#endif  // TRIE_HPP
