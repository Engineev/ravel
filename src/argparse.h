#pragma once

#include <string>
#include <vector>

namespace ravel {

struct Config {
  std::vector<std::string> files;
};

Config parseArgs(const std::vector<std::string> &args);

} // namespace ravel
