#pragma once

#include <functional>
#include <optional>
#include <type_traits>

namespace ravel {

template <class Set, class V> bool isIn(const Set &s, const V &val) {
  return s.find(val) != s.end();
}

template <class Container>
void append(Container &container, const Container &values) {
  container.insert(container.end(), values.begin(), values.end());
}

template <class Map, class Key>
std::optional<typename std::decay_t<Map>::mapped_type> get(Map &&mp,
                                                           const Key &key) {
  auto iter = mp.find(key);
  if (iter == mp.end())
    return std::nullopt;
  return mp.at(key);
}

} // namespace ravel

namespace ravel {

template <class T> class Id {
  template <class K> friend struct std::hash;

public:
  Id() {
    static int currentId = 1;
    val = currentId++;
  }

  bool operator==(const Id &rhs) const { return val == rhs.val; }

  bool operator!=(const Id &rhs) const { return !(*this == rhs); }

private:
  int val;
};

} // namespace ravel

namespace std {
template <class T> struct hash<ravel::Id<T>> {
  using Key = ravel::Id<T>;

  std::size_t operator()(const Key &k) const { return std::hash<int>()(k.val); }
};
} // namespace std
