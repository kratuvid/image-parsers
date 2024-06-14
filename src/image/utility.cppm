export module image:utility;

import <exception>;
import <string_view>;
import <stdexcept>;

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
