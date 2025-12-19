#include <source_location>
#include <sstream>
#include <stdexcept>

[[noreturn]] inline void throw_runtime_error(
    const std::string& msg,
    const std::source_location& loc = std::source_location::current()) {
  std::ostringstream oss;
  oss << "Exception: " << msg << "\n"
      << "  File: " << loc.file_name() << "\n"
      << "  Line: " << loc.line() << "\n"
      << "  Function: " << loc.function_name();

  throw std::runtime_error(oss.str());
}
