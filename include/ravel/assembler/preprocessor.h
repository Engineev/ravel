#pragma once

#include <string>
#include <vector>

namespace ravel {

// Split the source file into lines.
// Remove comments.
// Trim each line.
// Make each label to occupy an entire line.
// Remove empty lines.
// Translate pseudo instructions.
std::vector<std::string> preprocess(const std::string &src);

} // namespace ravel
