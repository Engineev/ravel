#pragma once

#include <exception>
#include <vector>

#include "assembler/object_file.h"
#include "interpretable.h"

namespace ravel {

class LinkError : public std::exception {
public:
  ~LinkError() override = default;
};

class UndefinedSymbol : public LinkError {
public:
  UndefinedSymbol(std::string symbol) : symbol(std::move(symbol)) {}
  ~UndefinedSymbol() override = default;

  const char *what() const noexcept override {
    return ("Undefined symbol: " + symbol).c_str();
  }

private:
  std::string symbol;
};

class DuplicatedSymbols : public LinkError {
public:
  DuplicatedSymbols(std::string symbol) : symbol(std::move(symbol)) {}
  ~DuplicatedSymbols() override = default;

  const char *what() const noexcept override {
    return ("Duplicated symbols: " + symbol).c_str();
  }

private:
  std::string symbol;
};

class MainNotFound : public LinkError {
public:
  ~MainNotFound() override = default;

  const char *what() const noexcept override {
    return "Cannot find label \"main\"";
  }
};

Interpretable link(const std::vector<ObjectFile> &objects);

} // namespace ravel
