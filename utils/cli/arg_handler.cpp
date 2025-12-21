#include <argp.h>

#include <complex>
#include <cstring>
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

int ArgpHandler::run_with_subcommands(int argc, char** argv) {
  if (argc <= 0) return -1;

  prepare_root_help();

  int cmd_idx = -1;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--") == 0) {
      if (i + 1 < argc) cmd_idx = i + 1;
      break;
    }
    if (argv[i][0] != '-') {
      cmd_idx = i;
      break;
    }
  }

  int global_argc = (cmd_idx == -1) ? argc : cmd_idx;

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

  int ret =
      argp_parse(&root_argp_, global_argc, argv, config.flags, nullptr, this);

  if (ret != 0) return ret;

  if (cmd_idx == -1) return 0;

  const char* cmd = argv[cmd_idx];
  auto sc = find_subcommand(cmd);

  if (!sc) {
    fprintf(stderr, "Unknown command: %s\n\nAvailable commands:\n", cmd);
    for (auto& s : subcommands_) {
      fprintf(stderr, "  %-16s %s\n", s.name.c_str(), s.doc.c_str());
    }
    return -1;
  }

  int sub_argc = argc - cmd_idx;
  char** sub_argv = argv + cmd_idx;

  ret =
      argp_parse(&sc->sub_argp, sub_argc, sub_argv, ARGP_NO_EXIT, nullptr, &sc);

  if (ret != 0) return ret;

  if (sc->handler) return sc->handler(sub_argc, sub_argv);

  return 0;
}

int ArgpHandler::default_parser(int key, char* arg, argp_state* state) {
  auto handler = static_cast<ArgpHandler*>(state->input);

  switch (key) {
    case 'v':
      printf("%s version %s\n", handler->config.program_name,
             handler->config.program_version);
      exit(0);
    case 'l':
      if (arg != nullptr) {
        std::string log_dir{arg};
        logs::log(INFO, "Initializing logs in dir [%s]...\n", log_dir.c_str());
        logs::init(log_dir);
      } else {
        logs::log(INFO, "Initializing logs in dir [%s]...\n", logs::LOG_DIR);
        logs::init(logs::LOG_DIR);
      }
      break;
    case ARGP_KEY_END:
      return 0;
    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

bool ArgpHandler::register_subcommand(const Subcommand sc) {
  for (auto const& c : subcommands_) {
    if (c.name == sc.name) {
      return false;
    }
  }

  subcommands_.push_back(sc);
  return true;
}

std::optional<ArgpHandler::Subcommand> ArgpHandler::find_subcommand(
    const char* command) {
  for (auto& s : subcommands_) {
    if (s.name == command) return s;
  }

  return std::nullopt;
}

void ArgpHandler::prepare_root_help() {
  std::string doc{config.doc};

  doc += "\n\nCommands:\n";
  for (auto& sc : subcommands_) {
    doc += "  " + sc.name;
    if (!sc.doc.empty()) {
      doc += std::string(12 - sc.name.size(), ' ');
      doc += sc.doc;
    }
    doc += "\n";
  }

  help_doc_storage_ = std::move(doc);
  root_argp_.doc = help_doc_storage_.c_str();
  if (root_argp_.args_doc == nullptr)
    root_argp_.args_doc = "command [CMD OPTONS...]";
}

}  // namespace fsatutils
