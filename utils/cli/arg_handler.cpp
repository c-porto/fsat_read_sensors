#include <argp.h>

#include <utils/cli/arg_handler.hpp>
#include <utils/log/log.hpp>

namespace fsatutils {

ArgpHandler::ArgpHandler(const Config config) : config{config} {
  root_argp_.parser =
      (config.parser != nullptr) ? config.parser : &ArgpHandler::default_parser;
  root_argp_.doc = config.doc;
  root_argp_.args_doc = config.args_doc;
  root_argp_.argp_domain = nullptr;
  root_argp_.help_filter = nullptr;
}

void ArgpHandler::add_child_structures(std::vector<argp_child> const& child) {
  child_array_.insert(child_array_.end(), child.begin(), child.end());
}

void ArgpHandler::add_global_options(std::vector<argp_option> const& options) {
  global_options_.insert(global_options_.end(), options.begin(), options.end());
}

int ArgpHandler::parse(int argc, char** argv) {
  struct argp_child terminator{
      .argp = nullptr,
      .flags = 0,
      .header = nullptr,
      .group = 0,
  };
  struct argp_option op_terminator{
      .name = nullptr,
      .key = 0,
      .arg = nullptr,
      .flags = 0,
      .doc = nullptr,
      .group = 0,
  };

  child_array_.push_back(terminator);
  global_options_.push_back(op_terminator);

  root_argp_.options = global_options_.data();
  root_argp_.children = child_array_.data();

  return argp_parse(&root_argp_, argc, argv, config.flags, 0, this);
}

int ArgpHandler::default_parser(int key, char* arg, argp_state* state) {
  (void)arg;
  auto handler = static_cast<ArgpHandler*>(state->input);

  switch (key) {
    case 'v':
      logs::log(INFO, "%s version %s\n", handler->config.program_name,
                handler->config.program_version);
      exit(0);
    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

}  // namespace fsatutils
