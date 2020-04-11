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

std::vector<std::string> tokenize(const std::string &line);

bool isDirective(const std::string &str);

bool isLabel(const std::string &str);

// If [line] emits or makes current a new section, then return
// the name of the section
std::optional<std::string> getSectionName(const std::string &line);

std::string opType2Name(inst::Instruction::OpType op);

inst::Instruction::OpType name2OpType(std::string name);

std::size_t regName2regNumber(const std::string &name);

std::string regNumber2regName(const std::size_t &num);

std::pair<std::size_t, int> parseBaseOffset(const std::string &str);

std::string toString(const std::shared_ptr<inst::Instruction> &inst);

// return the section name (e.g. .text)
std::string parseSectionDerivative(const std::string &line);

std::uint32_t parseImm(const std::string &str);

std::string handleEscapeCharacters(const std::string &str);

} // namespace ravel
