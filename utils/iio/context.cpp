#include <iio.h>

#include <string>
#include <utils/errors.hpp>
#include <utils/iio/context.hpp>

namespace fsatutils {

namespace iio {

Context::Context(ContextType type) {
  switch (type) {
    case ContextType::NETWORK:
      raw_ = iio_create_network_context("127.0.0.1");
      type_ = ContextType::NETWORK;
      break;
    case ContextType::LOCAL:
      raw_ = iio_create_local_context();
      type_ = ContextType::LOCAL;
      break;
    case ContextType::USB: /* Not implemented yet */
    case ContextType::XML: /* Not implemented yet */
    case ContextType::DEFAULT:
      raw_ = iio_create_default_context();
      type_ = ContextType::DEFAULT;
      break;
  }

  if (raw_ == nullptr) {
    throw_runtime_error("Failed to create IIO Context!");
  }
}

Context::Context(ContextType type, std::string& uri) {
  switch (type) {
    case ContextType::NETWORK:
      raw_ = iio_create_network_context(uri.c_str());
      type_ = ContextType::NETWORK;
      break;
    case ContextType::LOCAL:
      raw_ = iio_create_local_context();
      type_ = ContextType::LOCAL;
      break;
    case ContextType::USB: /* Not implemented yet */
    case ContextType::XML: /* Not implemented yet */
    case ContextType::DEFAULT:
      raw_ = iio_create_default_context();
      type_ = ContextType::DEFAULT;
      break;
  }

  if (raw_ == nullptr) {
    throw_runtime_error("Failed to create IIO Context!");
  }
}

Context::~Context() {
  if (raw_ != nullptr) {
    iio_context_destroy(raw_);
    raw_ = nullptr;
  }
}

}  // namespace iio

}  // namespace fsatutils
