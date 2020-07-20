// MIT License
//
// Copyright (c) 2019 Quentin Balland
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

#include <string>

namespace fil {

class option {
public:
	explicit option(std::string name, std::string helper, std::function<void()> handler)
			:_name(std::move(name)), _helper(std::move(helper)), _hander(std::move(handler)) { }

	void add_sub_command(sub_command&& command)
	{
		_sub_commands.emplace_back(std::move(command));
	}

	void add_option(option&& opt)
	{
		_options.emplace_back(std::move(opt));
	}

private:
	std::string _name;
	std::string _helper;

	std::function<void()> _handler;
};

class sub_command {
public:
	explicit sub_command(std::string name, std::string helper, std::function<void()> handler)
			:_name(std::move(name)), _helper(std::move(helper)), _hander(std::move(handler)) { }

	void add_sub_command(sub_command&& command)
	{
		_sub_commands.emplace_back(std::move(command));
	}

	void add_option(option&& opt)
	{
		_options.emplace_back(std::move(opt));
	}

	bool exec_command(const std::vector<std::string>& args, std::uint32_t index)
	{
		do {
			if (args.at(index).front() == '-') {
				exec_argument(args, index);
			}
			else {

			}
		}
		while (++index < args.size());
		_handler();
		return true;
	}

private:
	std::string _name;
	std::string _helper;

	std::vector<sub_command> _sub_commands;

	std::vector<option> _options;

	std::function<void()> _handler;
};

class command_line_interface : public sub_command {
public:
	explicit command_line_interface(std::string helper, std::function<void()> handler)
			:sub_command({}, std::move(helper), std::move(handler)) { }

	void parse_command_line(int argc, char** argv)
	{
		std::vector<std::string> arguments{};

		if (argc > 1) {
			arguments = std::vector(argv + 1, argv + argc);
		}
		exec_command(arguments, 0u);
	}

};

}

#endif //FIL_COMMAND_LINE_INTERFACE_HH
