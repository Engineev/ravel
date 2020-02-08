#pragma once

#include <optional>
#include <type_traits>

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
