#pragma once

#include <cstring>
#include <string_view>
#include <functional>

struct CharPointerComp {
  bool operator()(const char* p1, const char* p2) const {
    return strcmp(p1, p2) == 0;
  }

  size_t operator()(const char* p) const {
    return std::hash<std::string_view>{}(p);
  }
};

struct Convertation {
  const char* from;
  const char* to;

  bool operator==(const Convertation& other) const {
    return (strcmp(from, other.from) == 0) && (strcmp(to, other.to) == 0);
  }
};

namespace std {

template <>
struct hash<Convertation> {
  std::size_t operator()(const Convertation& c) const {
    return std::hash<std::string_view>()(c.from) ^
           std::hash<std::string_view>()(c.to);
  }
};

}  // namespace std

using conversion_mapping = typename std::pair<Convertation, std::function<int(const char* from_file, const char* to_file)>>;
