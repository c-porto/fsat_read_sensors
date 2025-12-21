#ifndef ARG_HANDLER_HPP_
#define ARG_HANDLER_HPP_

#include <argp.h>

#include <functional>
#include <optional>
#include <string>
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

  struct Subcommand {
    std::string parent;
    std::string name;
    std::string doc;
    std::string args_doc;
    argp sub_argp{};
    argp_parser_t parser = nullptr;
    std::function<int(int, char**)> handler = nullptr;
    void* data;

    Subcommand() = default;
    Subcommand(const std::string parent, const std::string n,
               const std::string d, argp_parser_t p, std::string a, void* data)
        : parent(std::move(parent)),
          name(std::move(n)),
          doc(std::move(d)),
          args_doc(std::move(a)),
          parser(p),
          data(data) {
      sub_argp.parser = parser;
      sub_argp.doc = doc.c_str();
      sub_argp.args_doc = nullptr;
      sub_argp.argp_domain = nullptr;
    }
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

  int run_with_subcommands(int argc, char** argv);

  bool register_subcommand(const Subcommand sc);

  bool register_subcommand(std::string parent, std::string name,
                           std::string doc, argp_parser_t parser,
                           std::string args_doc,
                           std::function<int(int, char**)> handler = nullptr);

  std::optional<Subcommand> find_subcommand(const char* command);

  void prepare_root_help();

  static int default_parser(int key, char* arg, argp_state* state);

  Config config;

 private:
  std::vector<argp_option> global_options_{
      {"version", 'v', nullptr, 0, "Prints the program version", 0},
      {"log-dir", 'l', "dir", 0, "Log directory", 0}};
  std::vector<argp_child> child_array_ = {};
  std::vector<Subcommand> subcommands_;
  std::string help_doc_storage_;
  argp root_argp_ = {};
};

}  // namespace fsatutils

#endif
