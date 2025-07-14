// MIT License
//
// Copyright (c) 2025 Quentin Balland
// Repository : https://github.com/FreeYourSoul/FiL
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

#include <fil/algorithm/string.hh>
#include <functional>
#include <list>
#include <regex>
#include <stdexcept>
#include <string>

#include <fmt/format.h>

namespace fil {

static constexpr std::string padding_helper = "        ";

namespace internal {

static std::string polish_string(const std::string& str_to_polish, const std::string& padding_ref, int width) {
    std::string padding = padding_ref;
    std::string result;
    int width_padding = 0;
    for (std::size_t pos_start = 0; pos_start < str_to_polish.size(); pos_start += std::abs(width - width_padding)) {
        const auto s = str_to_polish.substr(pos_start, std::abs(width - width_padding));
        std::vector<std::string> vec_split_endline;
        split_string(s, std::string {"\n"}, [&](const auto& str_split_endline) { //
            vec_split_endline.emplace_back(str_split_endline);
        });

        if (vec_split_endline.size() <= 1) {
            result += fmt::format("{}{}\n", padding, s);
            padding       = padding_ref;
            width_padding = 0;
        } else {
            for (const auto& vec : vec_split_endline) {
                if (&vec == &vec_split_endline.front()) {
                    result += fmt::format("{}{}", padding, vec);
                } else {
                    result += fmt::format("\n{}{}", padding, vec);
                }
            }
            padding       = "";
            width_padding = std::ssize(vec_split_endline.back());
        }
    }

    return result;
}

class cli_base_action {
  public:
    explicit cli_base_action(std::string name, std::function<void()> handler, std::string helper = "")
        : name_(std::move(name))
        , helper_(std::move(helper))
        , handler_(std::move(handler)) {}

    explicit cli_base_action(std::string name, std::function<void(std::string)> handler, std::string helper = "")
        : name_(std::move(name))
        , helper_(std::move(helper))
        , handler_with_param_(std::move(handler)) {}

    void add_helper_usage(std::string usage) { usage_ = std::move(usage); }

    void exec() const {
        if (handler_) {
            called_ = true;
            std::invoke(handler_);
        }
    }

    void exec(std::string param) const {
        if (handler_with_param_) {
            called_ = true;
            std::invoke(handler_with_param_, std::move(param));
        }
    }

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] const std::string& alias() const { return alias_; }

    [[nodiscard]] bool has_param() const { return handler_with_param_ != nullptr; }

    //! retrieve a handler to know if the current action has been called after a parse command
    std::reference_wrapper<const bool> has_been_called_handler() const { return called_; }

  protected:
    std::string name_;  //!< name of the cli action, it is used to identify the action in the command line
    std::string alias_; //! alias of the cli action, it is used to identify the action in the command line, it can be empty
    std::string helper_;
    std::string usage_;

    mutable bool called_ = false;

  private:
    std::function<void()> handler_                       = nullptr; //!< handler to execute when the action is called without parameter
    std::function<void(std::string)> handler_with_param_ = nullptr; //!< handler to execute when the action is called with a parameter
};

} // namespace internal

class option : public internal::cli_base_action {
  public:
    option(std::string name, std::string alias, std::function<void()> handler, std::string helper = "")
        : cli_base_action(std::move(name), std::move(handler), std::move(helper)) {
        alias_ = std::move(alias);
    }
    option(std::string name, std::function<void()> h, std::string helper = "")
        : option(std::move(name), "", std::move(h), std::move(helper)) {}

    option(std::string name, std::string alias, std::function<void(std::string)> handler, std::string helper = "")
        : cli_base_action(std::move(name), std::move(handler), std::move(helper)) {
        alias_ = std::move(alias);
    }
    option(std::string name, std::function<void(std::string)> h, std::string helper = "")
        : option(std::move(name), "", std::move(h), std::move(helper)) {}

    option(std::string name, std::string alias, std::function<void(std::int64_t)> h, std::string helper = "")
        : cli_base_action(std::move(name), [h = std::move(h)](const std::string& str) { h(std::stol(str)); }, std::move(helper)) {
        alias_ = std::move(alias);
    }
    option(std::string name, std::function<void(std::int64_t)> h, std::string helper = "")
        : option(std::move(name), "", std::move(h), std::move(helper)) {}

    [[nodiscard]] std::string generate_helper() const {
        const std::string padded_option_name = [&] {
            std::string option_text;
            if (!alias_.empty()) {
                option_text = fmt::format("{}, {}", name_, alias_);
            } else {
                option_text = name_;
            }

            if (has_param()) {
                option_text += " <arg>";
            }
            return fmt::format(FMT_STRING("    {}\n"), option_text);
        }();

        return padded_option_name + padding_helper + helper_;
    }
};

/**
 * A subcommand is a command that can have multiple options, it can have arguments
 * if _sub_command_only is set, this subcommand can only call by via another embedded subcommand and cannot be called otherwise
 * In this case, options are still enabled.
 *
 * @example it is used as follow :
 * ./binary_executable subcommand --option_1 --option2-with-argument argument argumentofthesubcommand
 *
 */
class sub_command : public internal::cli_base_action {
  public:
    explicit sub_command(std::string name, std::function<void()> handler, std::string helper, std::list<sub_command> sub_commands = {},
                         std::list<option> options = {})
        : cli_base_action(std::move(name), std::move(handler), std::move(helper))
        , sub_commands_(std::move(sub_commands))
        , options_(std::move(options)) {
        usage_ = default_usage(*this, true);
        options_.emplace_back(option(
            "--help", "-h",
            [this] {
                fmt::print("{}", this->generate_helper());
                std::exit(0);
            },
            "Display this helper"));
    }

    explicit sub_command(std::string name, std::string helper, std::list<sub_command> sub_commands = {}, std::list<option> options = {})
        : cli_base_action(
              std::move(name), []() {}, std::move(helper))
        , sub_command_only_(true)
        , sub_commands_(std::move(sub_commands))
        , options_(std::move(options)) {
        usage_ = default_usage(*this, true);
        options_.emplace_back(option(
            "--help", "-h",
            [this] {
                fmt::print("{}", this->generate_helper());
                std::exit(0);
            },
            "Display this helper"));
    }

    void on_parameter_handler(std::function<void(std::string)> on_param) { _handler_on_param = std::move(on_param); }

    /**
     * @brief Add a pre-executed handler that will be called before the command is executed.
     * @note the main usage of such a haandler would be to use aggregated elements out of a succession of options and trigger a
     * pre-computation before executing a sub-command.
     * @param pre_executed handler that will be called before the command is executed
     */
    void add_pre_executed_handler(std::function<void()> pre_executed) { handler_pre_executed_ = std::move(pre_executed); }

    const bool& add_sub_command(const sub_command& command) {
        sub_commands_.emplace_back(command);
        return sub_commands_.back().has_been_called_handler();
    }

    const bool& add_option(option&& opt) {
        if (opt.name().front() != '-' || (!opt.alias().empty() && opt.alias().front() != '-')) {
            fmt::print("Error Option {} (alias '{}') : An option has to start with '-' character", opt.name(), opt.alias());
            throw std::invalid_argument("CLI error option should start with '-'");
        }
        options_.emplace_back(std::move(opt));
        return options_.back().has_been_called_handler();
    }

    void set_sub_command_only(bool is_sub_command_only) { sub_command_only_ = is_sub_command_only; }

  protected:
    friend std::string default_usage(const sub_command& command, bool is_a_subcommand) {
        const std::string prefix = (is_a_subcommand ? "... " : "") + command.name();
        std::string opt_desc     = internal::polish_string( //
            std::accumulate(command.options_.begin(), command.options_.end(), std::string {},
                                [](const auto& res, const option& opt) {
                                std::string p = opt.name();
                                if (!opt.alias().empty())
                                    p = opt.alias() + "|" + opt.name();
                                if (opt.has_param())
                                    p += " PARAM";
                                return res + " [" + p + "]";
                            }),
            std::string(prefix.size() + 2, ' '), 132 + prefix.size());
        ltrim(opt_desc);
        opt_desc.erase(std::ranges::find_if(opt_desc.rbegin(), opt_desc.rend(), //
                                            [](auto ch) { return !std::isspace(ch) && ch != '\n'; })
                           .base(),
                       opt_desc.end());

        std::string usage = prefix + " " + opt_desc;
        if (!command.sub_commands_.empty()) {
            usage += " (subcommand ... <see definitions below> )";
        }
        return usage + " ";
    }

    [[nodiscard]] bool exec_command(std::vector<std::string>& args, std::uint32_t index) {
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
                        throw std::invalid_argument(
                            fmt::format(FMT_STRING("CLI error usage Chaining command is impossible:\n{}"), generate_helper()));
                    }
                    if (handler_pre_executed_) {
                        std::invoke(handler_pre_executed_); // exec pre-execution before submodule execution
                    }
                    return exec_subcommand(args, index);
                }
            }
            ++index;
        }
        if (sub_command_only_) {
            throw std::invalid_argument(fmt::format(FMT_STRING("Command {} is subcommand only:\n{}"), name_, generate_helper()));
        }
        if (handler_pre_executed_) {
            std::invoke(handler_pre_executed_);
        }
        exec();
        return true;
    }

    [[nodiscard]] std::string generate_helper() const {
        std::string help =
            fmt::format(FMT_STRING("This a helper for the command {}:\n     {}\n{}\n"), name_,
                        usage_.empty() ? "" : fmt::format("\nUsage : {}\n", usage_), internal::polish_string(helper_, " ", 137));

        if (!options_.empty()) {
            help = fmt::format(FMT_STRING("{} * Available Options :"), help);
            for (const auto& opt : options_) {
                help = fmt::format(FMT_STRING("{}\n\n{}"), help, opt.generate_helper());
            }
        }
        if (!sub_commands_.empty()) {
            help = fmt::format(FMT_STRING("{}\n\n\n * Available Sub Commands:"), help);
            for (const auto& sub : sub_commands_) {
                help = fmt::format(FMT_STRING("{}\n\n    {} :\n{}"), help, sub.name_,
                                   internal::polish_string(sub.helper_, padding_helper, 130));
            }
        }
        return help.append("\n");
    }

  private:
    [[nodiscard]] bool is_not_command(const std::string& command_to_check) const {
        auto it = std::ranges::find_if(sub_commands_, [&command_to_check](const auto& value) { return value.name() == command_to_check; });
        return it == sub_commands_.end();
    }

    void exec_parameter(std::vector<std::string>& args, std::uint32_t index) {
        if (!_handler_on_param) {
            return;
        }
        if (index >= args.size() || args.at(index).front() == '-') {
            throw std::invalid_argument(
                fmt::format(FMT_STRING("error usage : command {} require a parameter :\n{}"), name_, generate_helper()));
        }
        _handler_on_param(std::move(args.at(index)));
    }

    bool exec_subcommand(std::vector<std::string>& args, std::uint32_t index) {
        auto it = std::ranges::find_if( //
            sub_commands_, [command_to_check = args.at(index)](const auto& value) { return value.name() == command_to_check; });
        if (it == sub_commands_.end()) {
            return false;
        }
        return it->exec_command(args, ++index);
    }

    void exec_option(std::vector<std::string>& args, std::uint32_t index) const {
        const std::string option_name = args.at(index);
        const auto it                 = std::ranges::find_if(
            options_, [&option_name](const auto& value) { return value.name() == option_name || value.alias() == option_name; });

        if (it == options_.end()) {
            throw std::invalid_argument(
                fmt::format(FMT_STRING("error usage : Option {} doesn't exists:\n{}"), option_name, generate_helper()));
        }
        if (it->has_param()) {
            ++index;
            if (index >= args.size()
                || (args.at(index).front() == '-' && !std::regex_match(args.at(index), std::regex("[(-|+)|][0-9]+")))) {
                throw std::invalid_argument(
                    fmt::format(FMT_STRING("error usage : Option {} require a parameter :\n{}"), option_name, it->generate_helper()));
            }
            it->exec(std::move(args.at(index)));
        } else {
            it->exec();
        }
    }

  private:
    bool sub_command_only_ = false;

    std::function<void()> handler_pre_executed_;        //!< handler that will be executed before the command is executed
    std::function<void(std::string)> _handler_on_param; //!< handlers that will be executed with a parameter

    // usage of a list instead of vector to not have a re-allocation of the elements when pushed, enforcing that the handlers
    // retrieved when call add_option or add_subcommand are not corrupted.
    // ps: command line parsing is far from being a hot path of an application, the loss of memory locality is better than a bad api
    std::list<sub_command> sub_commands_;
    std::list<option> options_;
};

/**
 * @brief Main class for a CLI, is a subclass of subcommand
 * This is the main entry point for a command line interface, it can be used to parse command line arguments, subcommands, and options are
 * provided at construction time to create a complex command line with sub-command in a git like manner.
 */
class command_line_interface : public sub_command {
  public:
    explicit command_line_interface(std::function<void()> handler, std::string helper = "")
        : sub_command({}, std::move(handler), std::move(helper)) {}

    explicit command_line_interface(std::list<sub_command> sub_commands, std::function<void()> handler, std::string helper = "")
        : sub_command({}, std::move(handler), std::move(helper), std::move(sub_commands)) {}

    bool parse_command_line(int argc, char** argv) {
        if (argc > 0) {
            name_ = std::string(argv[0]); //!< setup name as the first element
            if (name_.starts_with("./")) {
                name_.erase(0, 2);
            }
            usage_ = default_usage(*this, false);
            std::vector<std::string> arguments(argv, argv + argc);
            return exec_command(arguments, 1u);
        }
        return false;
    }
};

namespace cli {

/**
 * Add an option storing the argument into an output parameter
 *
 * @param sub_command add the option in this sub_command
 * @param opt option code of the option
 * @param argument output parameter in which storing the argument of the option (can be integral or string)
 * @param help helping string displayed when using --help
 */
template<typename T>
[[maybe_unused]] const bool& add_argument_option(sub_command& sub_command, auto opt, T& argument, std::string help = "") {
    return sub_command.add_option(option(std::move(opt), [&argument](T arg) { argument = std::move(arg); }, std::move(help)));
}
/**
 *
 * Add an option storing the argument into an output parameter (with an alias)
 *
 * @param sub_command add the option in this sub_command
 * @param opt option code of the option
 * @param alias alias of the option (can be empty)
 * @param argument output parameter in which storing the argument of the option (can be integral or string)
 * @param help helping string displayed when using --help
 */
template<typename T>
[[maybe_unused]] const bool& add_aliased_argument_option( //
    sub_command& sub_command, std::string opt, std::string alias, T& argument, std::string help = "") {
    return sub_command.add_option(
        option(std::move(opt), std::move(alias), [&argument](T arg) { argument = std::move(arg); }, std::move(help)));
}

/**
 * Do an aggregation of the argument of a sub_command into a vector
 *
 * @param sub_command command to do the aggregation from
 * @param args_string output parameter : aggregate all the parameter into this vector
 */
[[maybe_unused]] inline void add_multi_arg(sub_command& sub_command, std::vector<std::string>& args_string) {
    sub_command.on_parameter_handler([&args_string](std::string arg) { args_string.emplace_back(std::move(arg)); });
}

} // namespace cli

} // namespace fil

#endif // FIL_COMMAND_LINE_INTERFACE_HH
