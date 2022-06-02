#pragma once

#include <exception>
#include <vector>

#include "interpretable.h"
#include "ravel/assembler/object_file.h"

namespace ravel {

Interpretable link(const std::vector<ObjectFile> &objects);

} // namespace ravel
