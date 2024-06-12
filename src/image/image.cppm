export module image;

import <print>;
import <format>;
import <exception>;
import <string_view>;
import <string>;
import <cstdint>;
import <vector>;
import <variant>;
import <memory>;
import <fstream>;
import <sstream>;
import <cstring>;
import <algorithm>;
import <limits>;
import <utility>;

import logger;

namespace image
{
	export class exception : public std::runtime_error
	{
	public:
		exception(std::string_view msg)
			:std::runtime_error(msg.data())
		{}
	};
};

#include "netpbm.inl"
