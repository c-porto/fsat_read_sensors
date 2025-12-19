#ifndef CONTEXT_HPP_
#define CONTEXT_HPP_

#include <iio.h>

#include <string>

namespace fsatutils {

namespace iio {

enum class ContextType {
  NETWORK,
  LOCAL,
  USB,
  XML,
  DEFAULT,
};

class Context {
 public:
  Context(ContextType type);
  Context(ContextType type, std::string&);
  ~Context();

  ContextType type() const { return type_; };

  operator struct iio_context*() { return raw_; };

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  Context(Context&&) = delete;
  Context& operator=(Context&&) = delete;

 private:
  struct iio_context* raw_;
  ContextType type_;
};

}  // namespace iio

}  // namespace fsatutils

#endif
