#pragma once

#include <exception>
#include <vector>

#include "assembler/object_file.h"
#include "interpretable.h"

namespace ravel {

Interpretable link(const std::vector<ObjectFile> &objects);

} // namespace ravel
