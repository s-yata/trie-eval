#include <algorithm>
#include <cassert>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "louds-trie.hpp"

namespace {

using namespace std;
using namespace std::chrono;
using namespace trie_eval;

string uint_str(uint64_t v) {
  char buf[24];
  int len = snprintf(buf, 24, "%lu", v);
  string str;
  for (int i = 0; i < len; ++i) {
    str += buf[i];
    if (i != len - 1 && (len - i) % 3 == 1) {
      str += ',';
    }
  }
  return str;
}

vector<string> read_keys() {
  printf("keys:\n");
  vector<string> keys;
  uint64_t sum = 0, min_len = UINT64_MAX, max_len = 0;
  string line;
  while (getline(cin, line)) {
    keys.push_back(line);
    sum += line.length();
    if (line.length() < min_len) {
      min_len = line.length();
    }
    if (line.length() > max_len) {
      max_len = line.length();
    }
  }
  assert(!keys.empty());
  printf(" #keys: %s\n", uint_str(keys.size()).c_str());
  double avg = (double)sum / keys.size();
  printf(" size: %s bytes (%.3f bytes/key in range [%lu, %lu])\n",
    uint_str(sum).c_str(),  avg, min_len, max_len);
  return move(keys);
}

void sort_and_uniquify_keys(vector<string> &keys) {
  sort(keys.begin(), keys.end());
  auto end = unique(keys.begin(), keys.end());
  keys.erase(end, keys.end());

  printf("unique_keys:\n");
  uint64_t sum = 0;
  for (auto it = keys.begin(); it != keys.end(); ++it) {
    sum += it->length();
  }
  printf(" #keys: %s bytes\n", uint_str(keys.size()).c_str());
  double avg = (double)sum / keys.size();
  printf(" size: %s bytes (%.3f bytes/key)\n",
    uint_str(sum).c_str(),  avg);
}

template <typename T>
void eval(const vector<string> &keys, const vector<string> &shuffled_keys,
  const vector<uint64_t> shuffled_ids) {
  T trie;
  printf("%s:\n", trie.name());

  high_resolution_clock::time_point begin = high_resolution_clock::now();
  trie.build(keys);
  high_resolution_clock::time_point end = high_resolution_clock::now();
  double elapsed = (double)duration_cast<nanoseconds>(end - begin).count();
  printf(" size: %s bytes (%.3f bytes/key)\n",
    uint_str(trie.size()).c_str(), (double)trie.size() / keys.size());
  printf(" build: elapsed = %.3f s (%.3f ns/key)\n",
    elapsed / 1000000000, elapsed / keys.size());

  begin = high_resolution_clock::now();
  vector<pair<uint64_t, string>> pairs;
  for (auto it = keys.begin(); it != keys.end(); ++it) {
    uint64_t id = trie.lookup(*it);
    assert(id != (uint64_t)-1);
    pairs.push_back(make_pair(id, *it));
  }
  sort(pairs.begin(), pairs.end(),
    [](const pair<uint64_t, string> &lhs, const pair<uint64_t, string> &rhs) {
      return lhs.first < rhs.first;
    });
  for (uint64_t id = 0; id < pairs.size(); ++id) {
    assert(id == pairs[id].first);
  }
  string key;
  for (auto it = pairs.begin(); it != pairs.end(); ++it) {
    trie.reverse_lookup(it->first, key);
    assert(key == it->second);
  }
  end = high_resolution_clock::now();
  elapsed = (double)duration_cast<nanoseconds>(end - begin).count();
  printf(" validation: %.3f s (%.3f ns/key)\n",
    elapsed / 1000000000, elapsed / keys.size());

  begin = high_resolution_clock::now();
  for (auto it = keys.begin(); it != keys.end(); ++it) {
    trie.lookup(*it);
  }
  end = high_resolution_clock::now();
  elapsed = (double)duration_cast<nanoseconds>(end - begin).count();
  printf(" lookup (sorted): %.3f s (%.3f ns/key)\n",
    elapsed / 1000000000, elapsed / keys.size());

  begin = high_resolution_clock::now();
  for (auto it = shuffled_keys.begin(); it != shuffled_keys.end(); ++it) {
    trie.lookup(*it);
  }
  end = high_resolution_clock::now();
  elapsed = (double)duration_cast<nanoseconds>(end - begin).count();
  printf(" lookup (shuffled): %.3f s (%.3f ns/key)\n",
    elapsed / 1000000000, elapsed / keys.size());

  begin = high_resolution_clock::now();
  for (uint64_t id = 0; id < keys.size(); ++id) {
    trie.reverse_lookup(id, key);
  }
  end = high_resolution_clock::now();
  elapsed = (double)duration_cast<nanoseconds>(end - begin).count();
  printf(" reverse_lookup (sorted): %.3f s (%.3f ns/key)\n",
    elapsed / 1000000000, elapsed / keys.size());

  begin = high_resolution_clock::now();
  for (auto it = shuffled_ids.begin(); it != shuffled_ids.end(); ++it) {
    trie.reverse_lookup(*it, key);
  }
  end = high_resolution_clock::now();
  elapsed = (double)duration_cast<nanoseconds>(end - begin).count();
  printf(" reverse_lookup (shuffled): %.3f s (%.3f ns/key)\n",
    elapsed / 1000000000, elapsed / keys.size());
}

void run() {
  ios_base::sync_with_stdio(false);

  vector<string> keys = read_keys();
  sort_and_uniquify_keys(keys);
  vector<string> shuffled_keys = keys;
  random_shuffle(shuffled_keys.begin(), shuffled_keys.end());
  vector<uint64_t> shuffled_ids(keys.size());
  generate(shuffled_ids.begin(), shuffled_ids.end(), []() {
    static uint64_t id = 0;
    return id++;
  });

  eval<LoudsTrie>(keys, shuffled_keys, shuffled_ids);
}

}  // namespace

int main() {
  run();
}
