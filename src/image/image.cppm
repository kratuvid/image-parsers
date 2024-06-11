export module image;

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

		template<class OverridedException, class... Args>
		void enact(std::string_view overrided_name, std::string_view format, Args&&... args)
		{
			const std::string real_format {std::format("image: {}: {}", overrided_name, format)};
			throw OverridedException(std::vformat(real_format, std::make_format_args(args...)));
		}
	};
};

#include "netpbm.inl"
