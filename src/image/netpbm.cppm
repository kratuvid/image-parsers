export module image:netpbm;

import <array>;
import <cstdint>;
import <cstring>;
import <format>;
import <fstream>;
import <memory>;
import <sstream>;
import <string_view>;

import logger;
import :utility;

namespace image
{
	export class netpbm
	{
	public: // variable declarations
		enum class tupltype_t
		{
			rgb = 1, bw, gray,
			rgb_alpha, bw_alpha, gray_alpha
		};
		static constexpr std::array<std::string_view, 6> str_to_tupltype {{
				"RGB", "BLACKANDWHITE", "GRAYSCALE",
				"RGB_ALPHA", "BLACKANDWHITE_ALPHA", "GRAYSCALE_ALPHA"
			}
		};

		struct header_t
		{
			uint16_t width, height;
			uint8_t depth, maxval;
			tupltype_t tupltype;

			bool incomplete()
			{
				return width == 0 || height == 0 || depth == 0 || maxval == 0 || static_cast<int>(tupltype) == 0;
			}
		};

		using data_t = std::unique_ptr<uint8_t>;

	private: // variables
		header_t header;
		data_t data;
		logger log {"netpbm"};

	public: // functions - construnction/destruction
		netpbm() { static_assert(sizeof(char) == 1); }
		netpbm(std::string_view path) { load(path); }

		void load(std::string_view path)
		{
			memset(&header, 0, sizeof(header_t));
			data.reset();

			std::ifstream file(path.data(), std::ios::in | std::ios::binary);
			if (!file.is_open())
				exception::enact("{}: failed to open", path);

			char magic[3];
			if (!file.read(magic, 3))
				exception::enact("{}: failed to read the magic number", path);
			if (!(magic[0] == 'P' && magic[1] == '7'))
				exception::enact("{}: illegal magic number 0x{:x}{:x}", path, magic[0], magic[1]);
			if (magic[2] != '\n')
				exception::enact("{}: Malformed PAM header. Expected a newline post the magic number", path);

			std::string line;
			while (std::getline(file, line))
			{
				if (line == "ENDHDR")
					break;

				std::string key, value;
				std::istringstream iss(line);
				iss >> key >> value;
				if (iss.fail()) break;

				if (key == "TUPLTYPE")
				{
					const auto it = std::find(str_to_tupltype.begin(), str_to_tupltype.end(), value);
					if (it == str_to_tupltype.end())
						exception::enact("{}: Unknown tupltype {}", path, value);
					const auto index = (it - str_to_tupltype.begin()) + 1;
					const auto tt = static_cast<enum tupltype_t>(index);
					header.tupltype = tt;
				}
				else
				{
					uint16_t value_u16;
					std::istringstream iss_local(value);
					if (!(iss_local >> value_u16))
						exception::enact("{}: Failed to convert header key {}\'s value {} to an unsigned 16-bit integer", path, key, value);
					if ((key == "DEPTH" || key == "MAXVAL") && (value_u16 > 255))
						exception::enact("{}: Out of bounds value for header element, {}", path, line);

					if (key == "WIDTH") header.width = value_u16;
					else if (key == "HEIGHT") header.height = value_u16;
					else if (key == "DEPTH") header.depth = value_u16;
					else if (key == "MAXVAL") header.maxval = value_u16;
				}
			}
			if (line != "ENDHDR")
				exception::enact("{}: Unexpected EOF while reading header", path);
			if (header.incomplete())
				exception::enact("{}: Some essential header elements are missing/invalid", path);
		}

	public: // miscellaneous
		class exception : public image::exception
		{
		public:
			exception(std::string_view msg) :image::exception(msg) {}
			
			template<class... Args>
			static void enact(const std::format_string<const Args&...>& format, const Args&... args)
			{
				throw exception(std::format(format, args...));
			}
		};
	};
};
