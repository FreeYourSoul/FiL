// MIT License
//
// Copyright (c) 2020 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FyS
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//         of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
//         to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//         copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
//         copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//         AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef FIL_COMMAND_LINE_INTERFACE_HH
#define FIL_COMMAND_LINE_INTERFACE_HH

#include <regex>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

namespace fil {

namespace internal {

class cli_base_action {
 public:
   explicit cli_base_action(std::string name, std::function<void()> handler, std::string helper = "")
	   : _name(std::move(name)), _helper(std::move(helper)), _handler(std::move(handler)) {}

   explicit cli_base_action(std::string name, std::function<void(std::string)> handler, std::string helper = "")
	   : _name(std::move(name)), _helper(std::move(helper)), _handler_with_param(std::move(handler)) {}

   void add_helper_usage(std::string usage) {
	  _usage = std::move(usage);
   }

   void exec() const {
	  if (_handler) {
		 _called = true;
		 fmt::print("call_handler exec & = {}\n", static_cast<void *>(&_called));
		 _handler();
	  }
   }

   void exec(std::string param) const {
	  if (_handler_with_param) {
		 _called = true;
		 _handler_with_param(std::move(param));
	  }
   }

   [[nodiscard]] const std::string&
   name() const { return _name; }

   [[nodiscard]] bool
   has_param() const { return _handler_with_param != nullptr; }

   //! retrieve a handler to know if the current action has been called after a parse command
   std::reference_wrapper<const bool> has_been_called_handler() const { return _called; }

 protected:
   std::string _name;
   std::string _helper;
   std::string _usage;

   mutable bool _called = false;

 private:
   std::function<void()> _handler = nullptr;
   std::function<void(std::string)> _handler_with_param = nullptr;
};

}// namespace internal

class option : public internal::cli_base_action {
 public:
   explicit option(std::string name, std::function<void()> handler, std::string helper = "")
	   : internal::cli_base_action(std::move(name), std::move(handler), std::move(helper)) {}

   explicit option(std::string name, std::function<void(std::string)> handler, std::string helper = "")
	   : internal::cli_base_action(std::move(name), std::move(handler), std::move(helper)) {}

   explicit option(std::string name, std::function<void(std::int64_t)> h, std::string helper = "")
	   : internal::cli_base_action(
		   std::move(name), [h = std::move(h)](const std::string& str) { h(std::stol(str)); }, std::move(helper)) {}

   [[nodiscard]] std::string
   generate_helper() const {
	  return fmt::format(FMT_STRING("{}{} : {}"), _name, has_param() ? "<arg>" : "", _helper);
   }
};

/**
 * A subcommand is a command that can have multiple options, it can have arguments
 * if _sub_command_only is set, this subcommand can only called by via another embedded subcommand and cannot be called otherwise
 * In this case, options are still enabled.
 *
 * @example it is used as follow :
 * ./binary_executable subcommand --option_1 --option2-with-argument argument argumentofthesubcommand
 *
 */
class sub_command : public internal::cli_base_action {
 public:
   explicit sub_command(std::string name, std::function<void()> handler, std::string helper,
						std::vector<sub_command> sub_commands = {}, std::vector<option> options = {})
	   : internal::cli_base_action(std::move(name), std::move(handler), std::move(helper)), _sub_command_only(false),
		 _sub_commands(std::move(sub_commands)), _options(std::move(options)) {
	  _options.emplace_back(option(
		  "--help", [this]() { fmt::print(this->generate_helper()); }, "Display this helper"));
   }

   //   explicit sub_command(std::string name, std::function<void(std::string)> handler, std::string helper,
   //						std::vector<sub_command> sub_commands = {}, std::vector<option> options = {})
   //	   : internal::cli_base_action(std::move(name), std::move(handler), std::move(helper)), _sub_command_only(false),
   //		 _sub_commands(std::move(sub_commands)), _options(std::move(options)) {
   //	  _options.emplace_back(option(
   //		  "--help", [this]() { fmt::print(this->generate_helper()); }, "Display this helper"));
   //   }

   explicit sub_command(std::string name, std::string helper,
						std::vector<sub_command> sub_commands = {}, std::vector<option> options = {})
	   : internal::cli_base_action(
		   std::move(name), []() {}, std::move(helper)),
		 _sub_command_only(true),
		 _sub_commands(std::move(sub_commands)), _options(std::move(options)) {
	  _options.emplace_back(option(
		  "--help", [this]() { fmt::print(this->generate_helper()); }, "Display this helper"));
   }

   void on_parameter_handler(std::function<void(std::string)> on_param) { _handler_on_param = std::move(on_param); }

   const bool& add_sub_command(const sub_command& command) {
	  _sub_commands.emplace_back(command);
	  return _sub_commands.back().has_been_called_handler();
   }

   const bool& add_option(option&& opt) {
	  if (opt.name().front() != '-') {
		 fmt::print("Error Option {} : An option has to start with '-' character", opt.name());
		 throw std::invalid_argument("CLI error option should start with '-'");
	  }
	  _options.emplace_back(std::move(opt));
	  return _options.back().has_been_called_handler();
   }

   void set_sub_command_only(bool is_sub_command_only) {
	  _sub_command_only = is_sub_command_only;
   }

 protected:
   [[nodiscard]] bool
   exec_command(std::vector<std::string>& args, std::uint32_t index) {
	  bool command_specific_started = false;

	  while (index < args.size()) {
		 if (args.at(index).front() == '-') {
			exec_option(args, index);
		 } else {
			if (is_not_command(args.at(index))) {
			   command_specific_started = true;
			   exec_parameter(args, index);
			} else {
			   if (command_specific_started) {
				  throw std::invalid_argument(fmt::format(FMT_STRING("CLI error usage Chaining command is impossible:\n{}"),
														  generate_helper()));
			   }
			   return exec_subcommand(args, index);
			}
		 }
		 ++index;
	  }
	  if (_sub_command_only) {
		 throw std::invalid_argument(fmt::format(FMT_STRING("Command {} is subcommand only:\n{}"),
												 _name,
												 generate_helper()));
	  }
	  exec();
	  return true;
   }

   [[nodiscard]] std::string generate_helper() const {
	  std::string help = fmt::format(
		  FMT_STRING("This a helper for the command {}:\n{}\n     {}\n"),
		  _name, _helper, _usage.empty() ? "" : fmt::format("Usage : {}", _usage));

	  if (!_options.empty()) {
		 help = fmt::format(FMT_STRING("{} options :"), help);
		 for (const auto& opt : _options) {
			help = fmt::format(FMT_STRING("{}\n\t{}"), help, opt.generate_helper());
		 }
	  }
	  if (!_sub_commands.empty()) {
		 help = fmt::format(FMT_STRING("{}\n\nsub commands:"), help);
		 for (const auto& sub : _sub_commands) {
			help = fmt::format(FMT_STRING("{}\n\t{} : {}"), help, sub._name, sub._helper);
		 }
	  }
	  return help.append("\n");
   }

 private:
   [[nodiscard]] bool
   is_not_command(const std::string& command_to_check) const {
	  auto it = std::find_if(_sub_commands.begin(), _sub_commands.end(),
							 [&command_to_check](const auto& value) { return value.name() == command_to_check; });
	  return it == _sub_commands.end();
   }

   void exec_parameter(std::vector<std::string>& args, std::uint32_t& index) {
	  if (!_handler_on_param) {
		 return;
	  }
	  if (index >= args.size() || args.at(index).front() == '-') {
		 throw std::invalid_argument(fmt::format(FMT_STRING("error usage : command {} require a parameter :\n{}"),
												 _name, generate_helper()));
	  }
	  _handler_on_param(std::move(args.at(index)));
   }

   bool exec_subcommand(std::vector<std::string>& args, std::uint32_t index) {
	  auto it = std::find_if(_sub_commands.begin(), _sub_commands.end(),
							 [command_to_check = args.at(index)](const auto& value) {
								return value.name() == command_to_check;
							 });
	  if (it == _sub_commands.end()) {
		 return false;
	  }
	  return it->exec_command(args, ++index);
   }

   void exec_option(std::vector<std::string>& args, std::uint32_t& index) const {
	  const std::string option_name = args.at(index);
	  auto it = std::find_if(_options.begin(), _options.end(),
							 [&option_name](const auto& value) { return value.name() == option_name; });

	  if (it == _options.end()) {
		 throw std::invalid_argument(fmt::format(FMT_STRING("error usage : Option {} doesn't exists:\n{}"),
												 option_name, generate_helper()));
	  }
	  if (it->has_param()) {
		 ++index;
		 if (index >= args.size() || (args.at(index).front() == '-' && !std::regex_match(args.at(index), std::regex("[(-|+)|][0-9]+")))) {
			throw std::invalid_argument(fmt::format(FMT_STRING("error usage : Option {} require a parameter :\n{}"),
													option_name, it->generate_helper()));
		 }
		 it->exec(std::move(args.at(index)));
	  } else {
		 it->exec();
	  }
   }

 private:
   bool _sub_command_only = false;

   std::function<void(std::string)> _handler_on_param;

   std::vector<sub_command> _sub_commands;
   std::vector<option> _options;
};

/**
 * @brief Main class for a CLI, is a subclass of subcommand
 */
class command_line_interface : public sub_command {
 public:
   explicit command_line_interface(std::function<void()> handler, std::string helper = "")
	   : sub_command({}, std::move(handler), std::move(helper)) {}

   explicit command_line_interface(std::vector<sub_command> sub_commands, std::function<void()> handler, std::string helper = "")
	   : sub_command({}, std::move(handler), std::move(helper), std::move(sub_commands)) {}

   bool parse_command_line(int argc, char** argv) {
	  if (argc > 0) {
		 std::vector<std::string> arguments(argv, argv + argc);
		 return exec_command(arguments, 1u);
	  }
	  return false;
   }
};

namespace cli {

/**
 * Add a an option storing the argument into an output parameter
 *
 * @param sub_command add the option in this sub_command
 * @param opt option code of the option
 * @param argument output parameter in which storing the argument of the option (can be integral or string)
 * @param help helping string displayed when using --help
 */
template<typename T>
[[maybe_unused]] static void add_argument_option(sub_command& sub_command, std::string opt, T& argument, std::string help = "") {
   sub_command.add_option(option(
	   std::move(opt),
	   [&argument](T arg) { argument = std::move(arg); },
	   std::move(help)));
}

/**
 * Do an aggregation of the argument of a sub_command into a vector
 *
 * @param sub_command command to do the aggregation from
 * @param args_string output parameter : aggregate all the parameter into this vector
 */
[[maybe_unused]] static void add_multi_arg(sub_command& sub_command, std::vector<std::string>& args_string) {
   sub_command.on_parameter_handler([&args_string](std::string arg) { args_string.emplace_back(std::move(arg)); });
}

}// namespace cli

}// namespace fil

#endif//FIL_COMMAND_LINE_INTERFACE_HH
