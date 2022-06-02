#pragma once

#include <string>

#include "ravel/assembler/object_file.h"

namespace ravel {

ObjectFile assemble(const std::string &src);

}
