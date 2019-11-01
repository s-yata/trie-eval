#include "louds-trie.hpp"

#include <algorithm>
#include <iostream>

namespace trie_eval {

LoudsTrie::LoudsTrie()
  : levels_(2), n_keys_(0), n_nodes_(1), size_(0), last_key_() {
  levels_[0].louds.add(0);
  levels_[0].louds.add(1);
  levels_[1].louds.add(1);
  levels_[0].outs.add(0);
  levels_[0].labels.push_back(' ');
}

void LoudsTrie::build(const vector<string> &keys) {
  for (auto it = keys.begin(); it != keys.end(); ++it) {
    add(*it);
  }
  uint64_t offset = 0;
  for (uint64_t i = 0; i < levels_.size(); ++i) {
    LoudsTrieLevel &level = levels_[i];
    level.louds.build();
    level.outs.build();
    offset += levels_[i].offset;
    level.offset = offset;
    size_ += level.size();
  }
}

uint64_t LoudsTrie::lookup(const string &query) const {
  // cout << "lookup: query = " << query << endl;
  if (query.length() >= levels_.size()) {
    return false;
  }
  uint64_t node_id = 0;
  uint64_t rank = 0;
  for (uint64_t i = 0; i < query.length(); ++i) {
    const LoudsTrieLevel &level = levels_[i + 1];
    if (rank != 0) {
      node_id = level.louds.select1(rank - 1) + 1;
      rank = node_id - rank;
    } else {
      node_id = 0;
    }
    for (uint8_t byte = query[i]; ; ++node_id, ++rank) {
      if (level.louds[node_id] || level.labels[rank] > byte) {
        return -1;
      }
      if (level.labels[rank] == byte) {
        break;
      }
    }
  }
  const LoudsTrieLevel &level = levels_[query.length()];
  if (!level.outs[rank]) {
    return -1;
  }
  return level.offset + level.outs.rank1(rank);
}

void LoudsTrie::reverse_lookup(uint64_t id, string &key) const {
  assert(id < n_keys());
  key.clear();
  uint64_t level_id = 0;
  while (id >= levels_[level_id + 1].offset) {
    ++level_id;
  }
  if (level_id == 0) {
    return;
  }
  id -= levels_[level_id].offset;
  for (uint64_t node_id = levels_[level_id].outs.select1(id); ; --level_id) {
    key.push_back(levels_[level_id].labels[node_id]);
    if (level_id == 1) {
      break;
    }
    uint64_t node_pos = levels_[level_id].louds.select0(node_id);
    node_id = node_pos - node_id;
  }
  reverse(key.begin(), key.end());
}

void LoudsTrie::add(const string &key) {
  assert(key > last_key_);
  if (key.empty()) {
    levels_[0].outs.set(0, 1);
    ++levels_[1].offset;
    ++n_keys_;
    return;
  }
  if (key.length() + 1 >= levels_.size()) {
    levels_.resize(key.length() + 2);
  }
  uint64_t i = 0;
  for ( ; i < key.length(); ++i) {
    LoudsTrieLevel &level = levels_[i + 1];
    uint8_t byte = key[i];
    if ((i == last_key_.length()) || (byte != level.labels.back())) {
      level.louds.set(levels_[i + 1].louds.n_bits - 1, 0);
      level.louds.add(1);
      level.outs.add(0);
      level.labels.push_back(key[i]);
      ++n_nodes_;
      break;
    }
  }
  for (++i; i < key.length(); ++i) {
    LoudsTrieLevel &level = levels_[i + 1];
    level.louds.add(0);
    level.louds.add(1);
    level.outs.add(0);
    level.labels.push_back(key[i]);
    ++n_nodes_;
  }
  levels_[key.length() + 1].louds.add(1);
  ++levels_[key.length() + 1].offset;
  levels_[key.length()].outs.set(levels_[key.length()].outs.n_bits - 1, 1);
  ++n_keys_;
  last_key_ = key;
}

}  // namespace trie_eval
