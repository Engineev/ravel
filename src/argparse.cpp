#include "argparse.h"

namespace ravel {

Config parseArgs(const std::vector<std::string> &args) {
  Config config;
  // TODO
  config.files.insert(config.files.end(), args.begin() + 1, args.end());
  return config;
}

} // namespace ravel
