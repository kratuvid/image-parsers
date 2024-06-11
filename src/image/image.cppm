export module image;

import <print>;
import <format>;
import <exception>;
import <string_view>;
import <string>;

namespace image
{
	export class exception : public std::runtime_error
	{
	public:
		exception(std::string_view msg) :std::runtime_error(msg.data()) {}
	};
};

#include "netpbm.inl"
