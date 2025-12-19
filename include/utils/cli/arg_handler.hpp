#ifndef ARG_HANDLER_HPP_
#define ARG_HANDLER_HPP_

#include <argp.h>

#include <vector>

namespace fsatutils {

template <typename T>
concept StaticArgpInterface = requires {
  { T::get_argp_children() } -> std::same_as<std::vector<argp_child>>;
};

template <typename T>
struct ArgpModule {
  ArgpModule() {
    static_assert(StaticArgpInterface<T>,
                  "Derived must implement: "
                  "static std::vector<argp_child> get_argp_children()");
  }
};

class ArgpHandler {
 public:
  struct Config {
    const char* program_name = nullptr;
    const char* program_version = nullptr;
    const char* args_doc = nullptr;
    const char* doc = nullptr;
    int flags = 0;
    int (*parser)(int key, char* arg, argp_state* state);
  };

  ArgpHandler(const Config config);
  ~ArgpHandler() = default;

  ArgpHandler(const ArgpHandler&) = delete;
  ArgpHandler& operator=(const ArgpHandler&) = delete;
  ArgpHandler(ArgpHandler&&) = delete;
  ArgpHandler& operator=(ArgpHandler&&) = delete;

  void add_child_structures(std::vector<argp_child> const& child);

  void add_global_options(std::vector<argp_option> const& options);

  int parse(int argc, char** argv);

  static int default_parser(int key, char* arg, argp_state* state);

  Config config;

 private:
  std::vector<argp_option> global_options_{
      {"version", 'v', nullptr, 0, "Prints the program version", 0}};
  std::vector<argp_child> child_array_ = {};
  argp root_argp_ = {};
};

}  // namespace fsatutils

#endif
