#pragma once

#include <exception>
#include <sstream>
#include <string>

namespace ravel {

class Exception : public std::exception {
public:
  explicit Exception(std::string msg) : msg(std::move(msg)) {}
  Exception(const Exception &o) noexcept { msg = o.msg; }
  ~Exception() override = default;

  const char *what() const noexcept override { return msg.c_str(); }

  void setMsg(const std::string &msg_) { msg = msg_; }

private:
  std::string msg;
};

class LinkError : public Exception {
public:
  explicit LinkError(const std::string &msg)
      : Exception("Link error: " + msg) {}
};

class UnresolvableSymbol : public LinkError {
public:
  explicit UnresolvableSymbol(const std::string &symbol)
      : LinkError("can not resolve symbol " + symbol) {}
};

class DuplicatedSymbols : public LinkError {
public:
  explicit DuplicatedSymbols(const std::string &symbol)
      : LinkError("duplicated symbols: " + symbol) {}
};

class NotSupportedError : public Exception {
public:
  using Exception::Exception;
};

class InvalidAddress : public Exception {
public:
  explicit InvalidAddress(std::size_t address) : Exception("") {
    std::stringstream ss;
    ss << std::hex << address;
    msg = ss.str();
  }

  const char *what() const noexcept override { return msg.c_str(); }

private:
  std::string msg;
};

class Timeout : public Exception {
  using Exception::Exception;
};

class RuntimeError : public Exception {
  using Exception::Exception;
};

} // namespace ravel