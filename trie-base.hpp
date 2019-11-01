#ifndef TRIE_BASE_HPP
#define TRIE_BASE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace trie_eval {

using namespace std;

class TrieBase {
 public:
  TrieBase() {}
  virtual ~TrieBase() {}

  virtual void build(const vector<string> &keys) = 0;

  virtual const char *name() const = 0;
  virtual uint64_t size() const = 0;

  virtual uint64_t lookup(const string &query) const = 0;
  virtual void reverse_lookup(uint64_t id, string &key) const = 0;
};

}  // namespace trie_eval

#endif  // TRIE_BASE_HPP
