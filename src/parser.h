#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "instructions.h"

// utilities
namespace ravel {

std::string strip(std::string str);

// split a string into words
std::vector<std::string> split(const std::string &s,
                               const std::string &delimiters = " \t");

bool isDirective(const std::string &str);

// If [line] emits or makes current a new section, then return
// the name of the section
std::optional<std::string> getSectionName(const std::string &line);

// Remove comments
// Split source code into lines
// Move labels into a separate line
// Remove empty lines
std::vector<std::string> preprocess(const std::string &src);

// All labels will be translated to the dummy offset 0x0.
std::shared_ptr<inst::Instruction>
parseInst(const std::vector<std::string> &tokens);

std::string opType2Name(inst::Instruction::OpType op);

inst::Instruction::OpType name2OpType(std::string name);

std::size_t regName2regNumber(const std::string &name);

std::pair<std::size_t, int> parseBaseOffset(const std::string &str);

std::string toString(const std::shared_ptr<inst::Instruction> &inst);

} // namespace ravel
