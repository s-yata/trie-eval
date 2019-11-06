#include "indirect.hpp"

#include <algorithm>
#include <queue>

namespace trie_eval {
namespace {

struct Trie {
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

  vector<Level> levels;
  string last_key;
  uint64_t n_keys;
  uint64_t n_nodes;

  Trie() : levels(2), last_key(), n_keys(0), n_nodes(1) {
    levels[0].louds.add(0);
    levels[0].louds.add(1);
    levels[1].louds.add(1);
    levels[0].outs.add(0);
    levels[0].labels.push_back(' ');
  }

  void add(const std::string &key) {
    assert(n_keys == 0 || key > last_key);
    if (key.empty()) {
      levels[0].outs.set(0, 1);
      ++n_keys;
      return;
    }
    if (key.length() + 1 >= levels.size()) {
      levels.resize(key.length() + 2);
    }
    uint64_t i = 0;
    for ( ; i < key.length(); ++i) {
      uint8_t byte = key[i];
      if ((i == last_key.length()) ||
          (byte != levels[i + 1].labels.back())) {
        levels[i + 1].louds.set(levels[i + 1].louds.n_bits - 1, 0);
        levels[i + 1].louds.add(1);
        levels[i + 1].outs.add(0);
        levels[i + 1].labels.push_back(key[i]);
        ++n_nodes;
        break;
      }
    }
    for (++i; i < key.length(); ++i) {
      levels[i + 1].louds.add(0);
      levels[i + 1].louds.add(1);
      levels[i + 1].outs.add(0);
      levels[i + 1].labels.push_back(key[i]);
      ++n_nodes;
    }
    levels[i + 1].louds.add(1);
    levels[i].outs.set(levels[i].outs.n_bits - 1, 1);
    last_key = key;
    ++n_keys;
  }

  void build() {
    for (uint64_t i = 0; i < levels.size(); ++i) {
      levels[i].louds.build();
    }
  }

  uint64_t size() const {
    uint64_t size = 0;
    for (uint64_t i = 0; i < levels.size(); ++i) {
      const Level &level = levels[i];
      size += level.louds.size();
      size += level.outs.size();
      size += level.labels.size();
    }
    return size;
  }
};

struct Node {
  uint64_t level_id:24;
  uint64_t node_pos:40;
};

struct Label {
  uint64_t link_id;
  string str;
};

}  // namespace

Indirect::Indirect()
  : louds_(), outs_(), link_bits_(), links_(), labels_(),
    tail_bits_(), tail_bytes_(), n_keys_(0), n_nodes_(0), size_(0) {}

void Indirect::build(const vector<string> &keys) {
  Trie trie;
  for (auto it = keys.begin(); it != keys.end(); ++it) {
    trie.add(*it);
  }
  trie.build();

  vector<Label> labels;
  Label label;
  uint64_t link_id = 0;

  louds_.add(0);
  louds_.add(1);
  outs_.add(trie.levels[0].outs[0]);
  link_bits_.add(0);
  labels_.push_back(' ');

  queue<Node> queue;
  if (!trie.levels[1].louds[0]) {
    queue.push(Node{ 1, 0 });
  }
  while (!queue.empty()) {
    Node node = queue.front();
    if (node.level_id != 0) {
      while (!trie.levels[node.level_id].louds[node.node_pos]) {
        louds_.add(0);
        uint64_t level_id = node.level_id;
        uint64_t node_pos = node.node_pos;
        uint64_t node_id = node_pos - trie.levels[level_id].louds.rank1(node_pos);
        labels_.push_back(trie.levels[level_id].labels[node_id]);
        for ( ; ; ) {
          node_pos = (node_id == 0) ? 0 :
            trie.levels[level_id + 1].louds.select1(node_id - 1) + 1;
          if (trie.levels[level_id].outs[node_id] ||
            !trie.levels[level_id + 1].louds[node_pos + 1]) {
            break;
          }
          node_id = node_pos - node_id;
          // tail_bits_.add(level_id == node.level_id);
          ++level_id;
          // tail_bytes_.push_back(trie.levels[level_id].labels[node_id]);
          label.str.push_back(trie.levels[level_id].labels[node_id]);
        }
        if (!trie.levels[level_id + 1].louds[node_pos]) {
          queue.push(Node{ level_id + 1, node_pos });
        } else {
          queue.push(Node{ 0, 0 });
        }
        link_bits_.add(level_id > node.level_id);
        if (level_id > node.level_id) {
          label.link_id = link_id++;
          labels.push_back(label);
          label.str.clear();
        }
        outs_.add(trie.levels[level_id].outs[node_id]);
        ++node.node_pos;
        ++node_id;
      }
    }
    louds_.add(1);
    queue.pop();
  }

  if (!labels.empty()) {
    sort(labels.begin(), labels.end(), [](const Label &lhs, const Label &rhs){
      return lhs.str < rhs.str;
    });
    uint64_t n_tails = 1;
    for (uint64_t i = 1; i < labels.size(); ++i) {
      if (labels[i].str != labels[i - 1].str) {
        ++n_tails;
      }
    }
    links_.init(labels.size(), n_tails - 1);
    uint64_t tail_id = 0;
    links_.set(labels[0].link_id, tail_id);
    tail_bits_.add(1);
    tail_bytes_.push_back(labels[0].str[0]);
    for (uint64_t i = 1; i < labels[0].str.size(); ++i) {
      tail_bits_.add(0);
      tail_bytes_.push_back(labels[0].str[i]);
    }
    for (uint64_t i = 1; i < labels.size(); ++i) {
      if (labels[i].str != labels[i - 1].str) {
        tail_bits_.add(1);
        tail_bytes_.push_back(labels[i].str[0]);
        for (uint64_t j = 1; j < labels[i].str.size(); ++j) {
          tail_bits_.add(0);
          tail_bytes_.push_back(labels[i].str[j]);
        }
        ++tail_id;
      }
      links_.set(labels[i].link_id, tail_id);
    }
  }

  louds_.build();
  outs_.build();
  link_bits_.build();
  tail_bits_.add(1);
  tail_bits_.build();

  n_keys_ = trie.n_keys;
  n_nodes_ = outs_.size();
  size_ = louds_.size();
  size_ += outs_.size();
  size_ += link_bits_.size();
  size_ += links_.size();
  size_ += labels_.size();
  size_ += tail_bits_.size();
  size_ += tail_bytes_.size();
}

// uint64_t Indirect::lookup(const string &query) const {
//   uint64_t node_id = 0;
//   for (uint64_t i = 0; i < query.length(); ++i) {
//     uint8_t byte = query[i];
//     uint64_t node_pos = louds_.select1(node_id) + 1;
//     node_id = node_pos - node_id - 1;
//     for ( ; ; ) {
//       if (louds_[node_pos]) {
//         return -1;
//       }
//       if (labels_[node_id] == byte) {
//         if (link_bits_[node_id]) {
//           uint64_t tail_id = links_[link_bits_.rank1(node_id)];
//           uint64_t tail_pos = tail_bits_.select1(tail_id);
//           for (++i; i < query.length(); ++i) {
//             if (tail_bytes_[tail_pos] != (uint8_t)query[i]) {
//               return -1;
//             }
//             ++tail_pos;
//             if (tail_bits_[tail_pos]) {
//               break;
//             }
//           }
//         }
//         break;
//       }
//       ++node_pos;
//       ++node_id;
//     }
//   }
//   if (!outs_[node_id]) {
//     return -1;
//   }
//   return outs_.rank1(node_id);
// }

uint64_t Indirect::lookup(const string &query) const {
  uint64_t node_id = 0;
  for (uint64_t i = 0; i < query.length(); ++i) {
    uint64_t node_pos = louds_.select1(node_id) + 1;

    uint64_t end = node_pos;
    uint64_t word = louds_.words[end / 64] >> (end % 64);
    if (word == 0) {
      end += 64 - (end % 64);
      word = louds_.words[end / 64];
      while (word == 0) {
        end += 64;
        word = louds_.words[end / 64];
      }
    }
    end += __builtin_ctzll(word);
    uint64_t begin = node_pos - node_id - 1;
    end = begin + end - node_pos;

    uint8_t byte = query[i];
    while (begin < end) {
      node_id = (begin + end) / 2;
      if (byte < labels_[node_id]) {
        end = node_id;
      } else if (byte > labels_[node_id]) {
        begin = node_id + 1;
      } else {
        if (link_bits_[node_id]) {
          uint64_t tail_id = links_[link_bits_.rank1(node_id)];
          uint64_t tail_pos = tail_bits_.select1(tail_id);
          for (++i; i < query.length(); ++i) {
            if (tail_bytes_[tail_pos] != (uint8_t)query[i]) {
              return -1;
            }
            ++tail_pos;
            if (tail_bits_[tail_pos]) {
              break;
            }
          }
          if (i == query.length()) {
            return -1;
          }
        }
        break;
      }
    }
    if (begin >= end) {
      return -1;
    }
  }
  if (!outs_[node_id]) {
    return -1;
  }
  return outs_.rank1(node_id);
}

void Indirect::reverse_lookup(uint64_t id, string &key) const {
  assert(id < n_keys());
  key.clear();
  uint64_t node_id = outs_.select1(id);
  while (node_id != 0) {
    if (link_bits_[node_id]) {
      uint64_t tail_id = links_[link_bits_.rank1(node_id)];
      uint64_t tail_pos = tail_bits_.select1(tail_id + 1);
      do {
        key.push_back(tail_bytes_[--tail_pos]);
      } while (!tail_bits_[tail_pos]);
    }
    key.push_back(labels_[node_id]);
    uint64_t node_pos = louds_.select0(node_id);
    node_id = node_pos - node_id - 1;
  }
  reverse(key.begin(), key.end());
}

}  // namespace trie_eval
