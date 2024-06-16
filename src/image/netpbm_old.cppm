export module image:netpbm;

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

import :utility;
import logger;

namespace image
{
	export class netpbm
	{
	public:
		struct common {
			size_t width, height;
			size_t bits;
			size_t bpp;
			std::vector<uint8_t> data;
		};
		struct pbm {
			struct common common;
		};
		struct pgm {
			struct common common;
			uint16_t max;
		};
		struct ppm {
			struct common common;
			uint16_t max;
		};

	private:
		std::variant<
			std::monostate,
			std::unique_ptr<pbm>, std::unique_ptr<pgm>, std::unique_ptr<ppm>> m_current;

		logger m_logger {"image:netpbm"};
		
	public:
		netpbm()
		{
		}
		
		netpbm(std::string_view path)
		{
			load(path);
		}

		template<class T>
		void assign(const T& data)
		{
			static_assert((typeid(T) == typeid(pbm))
						  || (typeid(T) == typeid(pgm))
						  || (typeid(T) == typeid(ppm)));
			
			m_current = std::make_unique<T>();
			auto& stored = *std::get<std::unique_ptr<T>>(m_current);
			stored.common = data.common;
			if constexpr (typeid(T) != typeid(pbm))
				stored.max = data.max;
		}

		void load(std::string_view path)
		{
			std::ifstream file(path.data(), std::ios::in | std::ios::binary);
			if (!file.is_open())
				exception::enact("Couldn't open '{}'", path);

			m_logger.debug("Parsing '{}'", path);

			char magic[3] {};
			if(!file.get(magic, 3) || file.eof())
				exception::enact("Failed to read magic number in '{}'", path);

			if (magic[0] != 'P')
				exception::enact("Unknown initial magic number '{}' in '{}'", magic[0], path);
			if (magic[1] == '1' || magic[1] == '4')
				load_pbm(file, magic[1] == '1' ? false : true, path);
			else if (magic[1] == '2' || magic[1] == '5')
				load_pgm(file, magic[1] == '2' ? false : true, path);
			else if (magic[1] == '3' || magic[1] == '6')
				load_ppm(file, magic[1] == '3' ? false : true, path);
			else
				exception::enact("Unknown last magic number '{}' in '{}'", magic[1], path);
		}

		void write(std::string_view path)
		{
			int index;
			if (path.ends_with(".pbm"))
				index = 1;
			else if (path.ends_with(".pgm"))
				index = 2;
			else if (path.ends_with(".ppm"))
				index = 3;
			else
				exception::enact("Unknown extension when writing '{}'", path);

			if (std::holds_alternative<std::monostate>(m_current))
				exception::enact("Encountered empty variant when writing '{}'", path);
			if (m_current.index() != index)
				exception::enact("Expected variant {}, got {} when writing '{}'", index, m_current.index(), path);

			std::ofstream file(path.data(), std::ios::out | std::ios::binary | std::ios::trunc);
			if (!file.is_open())
				exception::enact("Failed to open file '{}' for writing", path);

			switch(index)
			{
			case 1: write_pbm(file, path);
				break;
			case 2: write_pgm(file, path);
				break;
			case 3: write_ppm(file, path);
				break;
			default:
				std::unreachable();
			}
		}

	private:
		void load_pbm(std::ifstream& file, bool binary, std::string_view path)
		{
			m_current = std::make_unique<pbm>();
			auto& c_pbm = *std::get<std::unique_ptr<pbm>>(m_current);

			std::string line;
			while (std::getline(file, line))
			{
				if (line.size() > 0 && line[0] != '#')
				{
					std::istringstream iss(line);
					iss >> c_pbm.common.width >> c_pbm.common.height;
					if (!iss)
						exception::enact("Failed to extract width and height from '{}'", path);
					break;
				}
			}
			if (c_pbm.common.width == 0 || c_pbm.common.height == 0)
				exception::enact("Invalid width/height ({}/{}) specified in '{}'", c_pbm.common.width, c_pbm.common.height, path);

			c_pbm.common.bpp = 1;
			c_pbm.common.bits = c_pbm.common.width * c_pbm.common.height * c_pbm.common.bpp;

			m_logger.debug("Header: PBM ({}), width: {}, height: {}, bpp: {}", binary ? "binary" : "ascii", c_pbm.common.width, c_pbm.common.height, c_pbm.common.bpp);

			const size_t bits = c_pbm.common.bits;
			const size_t bytes = bits % 8 == 0 ? bits / 8 : (bits / 8) + 1;

			c_pbm.common.data.resize(bytes);
			memset(c_pbm.common.data.data(), 0, bytes);

			if (binary)
			{
				const auto prev_at = file.tellg();
				file.seekg(0, std::ios::end);
				const size_t bytes_available = file.tellg() - prev_at;
				file.seekg(prev_at);

				if (bytes > bytes_available)
					exception::enact("{}B of data missing in '{}'", bytes - bytes_available, path);

				static_assert(sizeof(char) == 1);

				char buffer[128];
				for (size_t at = 0; at < bytes; at += file.gcount())
				{
					if (!file || file.eof())
						exception::enact("Failed to read the rest of the {}B in the data section in '{}'", bytes - at, path);

					const size_t to_read = std::min(sizeof(buffer), bytes - at);
					file.read(buffer, to_read);
					memcpy(c_pbm.common.data.data() + at, buffer, to_read);
				}
			}
			else
			{
				exception::enact("PBM ascii parser is broken");

				size_t dest_at = 0, source_bit_at = 0;
				uint8_t current_byte = 0;

				while (bits > (dest_at * 8 + source_bit_at))
				{
					if (!file || file.eof())
						exception::enact("Failed to read the remaining {}B in '{}'", bytes - dest_at, path);

					char c = file.get();
					if (!isspace(c))
					{
						const uint8_t bit = '1' - c;
						current_byte |= bit << source_bit_at;
						
						c_pbm.common.data[dest_at] = current_byte;

						source_bit_at++;
						if (source_bit_at >= 8)
						{
							source_bit_at = 0;
							current_byte = 0;
							dest_at++;
						}
					}
				}
			}

			m_logger.debug("Size: struct: {}B, data: {}B = {}B", sizeof(pbm), c_pbm.common.data.size(), sizeof(pbm) + c_pbm.common.data.size());
		}

		void load_pgm(std::ifstream& file, bool binary, std::string_view path)
		{
			m_current = std::make_unique<pgm>();
			auto& c_pgm = *std::get<std::unique_ptr<pgm>>(m_current);

			std::string line;
			bool is_hdr_dimensions = false;
			while (std::getline(file, line))
			{
				if (line.size() > 0 && line[0] != '#')
				{
					std::istringstream iss(line);
					if (!is_hdr_dimensions)
					{
						iss >> c_pgm.common.width >> c_pgm.common.height;
						if (!iss)
							exception::enact("Failed to extract width and height from '{}'", path);
						is_hdr_dimensions = true;
					}
					else
					{
						iss >> c_pgm.max;
						if (!iss)
							exception::enact("Failed to extract max value from '{}'", path);
						break;
					}
				}
			}
			if (c_pgm.common.width == 0 || c_pgm.common.height == 0)
				exception::enact("Invalid width/height ({}/{}) specified in '{}'", c_pgm.common.width, c_pgm.common.height);
			if (c_pgm.max == 0)
				exception::enact("Invalid max value ({}) specified in '{}'", c_pgm.max, path);

			c_pgm.common.bpp = c_pgm.max > 255 ? 16 : 8;
			c_pgm.common.bits = c_pgm.common.width * c_pgm.common.height * c_pgm.common.bpp;

			m_logger.debug("Header: PGM ({}), width: {}, height: {}, max: {}, bpp: {}", binary ? "binary" : "ascii", c_pgm.common.width, c_pgm.common.height, c_pgm.max, c_pgm.common.bpp);

			const size_t bits = c_pgm.common.bits;
			const size_t bytes = bits / 8;
			const uint16_t factor = c_pgm.common.bpp == 16 ?
				std::numeric_limits<uint16_t>::max() / c_pgm.max :
				std::numeric_limits<uint8_t>::max() / uint8_t(c_pgm.max);

			c_pgm.common.data.resize(bytes);
			memset(c_pgm.common.data.data(), 0, bytes);

			auto data = c_pgm.common.data.data();
			const size_t count = c_pgm.common.bpp == 16 ? 2 : 1;
			if (binary)
			{
				const auto data_at = file.tellg();
				file.seekg(0, std::ios::end);
				const size_t bytes_available = file.tellg() - data_at;
				file.seekg(data_at);

				if (bytes > bytes_available)
					exception::enact("{}B of data missing in '{}'", bytes - bytes_available, path);

				static_assert(sizeof(char) == 1);

				for (size_t at = 0; at < bytes;)
				{
					if (!file || file.eof())
						exception::enact("Failed to read the rest of the {}B in the data section in '{}'", bytes - at, path);
					if (at + count > bytes_available)
						exception::enact("Not enough data left in '{}'", path);

					uint16_t unit = 0;
					file.read(reinterpret_cast<char*>(&unit), count);

					const auto pixel = unit * factor;
					data[at++] = pixel & 0xff;
					if (count == 2)
						data[at++] = pixel >> 8;
				}
			}
			else
			{
				for (size_t at = 0; at < bytes;)
				{
					uint16_t unit = 0;
					file >> unit;

					if (!file || file.eof())
						exception::enact("Failed to read the remaining {}B in '{}'", bytes - at, path);

					const uint16_t pixel = unit * factor;
					data[at++] = pixel & 0xff;
					if (count == 2)
						data[at++] = pixel >> 8;
				}
			}

			m_logger.debug("Size: struct: {}B, data: {}B = {}B", sizeof(pgm), c_pgm.common.data.size(), sizeof(pgm) + c_pgm.common.data.size());
		}

		void load_ppm(std::ifstream& file, bool binary, std::string_view path)
		{
			m_current = std::make_unique<ppm>();
			auto& c_ppm = *std::get<std::unique_ptr<ppm>>(m_current);

			std::string line;
			bool is_hdr_dimensions = false;
			while (std::getline(file, line))
			{
				if (line.size() > 0 && line[0] != '#')
				{
					std::istringstream iss(line);
					if (!is_hdr_dimensions)
					{
						iss >> c_ppm.common.width >> c_ppm.common.height;
						if (!iss)
							exception::enact("Failed to extract width and height from '{}'", path);
						is_hdr_dimensions = true;
					}
					else
					{
						iss >> c_ppm.max;
						if (!iss)
							exception::enact("Failed to extract max value from '{}'", path);
						break;
					}
				}
			}
			if (c_ppm.common.width == 0 || c_ppm.common.height == 0)
				exception::enact("Invalid width/height ({}/{}) specified in '{}'", c_ppm.common.width, c_ppm.common.height);
			if (c_ppm.max != 255)
				exception::enact("Unsupported max value specified in '{}'", c_ppm.max, path);

			c_ppm.common.bpp = 1 * 8 * 3;
			c_ppm.common.bits = c_ppm.common.width * c_ppm.common.height * c_ppm.common.bpp;

			m_logger.debug("Header: PPM ({}), width: {}, height: {}, max: {}, bpp: {}", binary ? "binary" : "ascii", c_ppm.common.width, c_ppm.common.height, c_ppm.max, c_ppm.common.bpp);

			const size_t bits = c_ppm.common.bits;
			const size_t bytes = bits / 8;

			c_ppm.common.data.resize(bytes);
			memset(c_ppm.common.data.data(), 0, bytes);

			auto data = c_ppm.common.data.data();
			if (binary)
			{
				const auto data_at = file.tellg();
				file.seekg(0, std::ios::end);
				const size_t bytes_available = file.tellg() - data_at;
				file.seekg(data_at);

				if (bytes > bytes_available)
					exception::enact("{}B of data missing in '{}'", bytes - bytes_available, path);

				static_assert(sizeof(char) == 1);

				for (size_t at = 0; at < bytes;)
				{
					if (!file || file.eof())
						exception::enact("Failed to read the rest of the {}B in the data section in '{}'", bytes - at, path);
					if (at + 3 > bytes_available)
						exception::enact("Not enough data left in '{}'", path);

					uint32_t unit = 0;
					file.read(reinterpret_cast<char*>(&unit), 3);

					const auto pixel = unit;
					data[at++] = pixel & 0xff;
					data[at++] = (pixel >> 8) & 0xff;
					data[at++] = (pixel >> 16) & 0xff;
				}
			}
			else
			{
				for (size_t at = 0; at < bytes;)
				{
					uint32_t unit[3] {};
					file >> unit[0] >> unit[1] >> unit[2];

					if (!file || file.eof())
						exception::enact("Failed to read the remaining {}B in '{}'", bytes - at, path);

					data[at++] = unit[0];
					data[at++] = unit[1];
					data[at++] = unit[2];
				}
			}

			m_logger.debug("Size: struct: {}B, data: {}B = {}B", sizeof(ppm), c_ppm.common.data.size(), sizeof(ppm) + c_ppm.common.data.size());
		}

	private:
		void write_pbm(std::ofstream& file, std::string_view path)
		{
			const auto& c_pbm = *std::get<std::unique_ptr<pbm>>(m_current);
			file << "P4" << '\n';
			write_common(file, c_pbm.common);
			write_data(file, c_pbm.common.data);
		}

		void write_pgm(std::ofstream& file, std::string_view path)
		{
			const auto& c_pgm = *std::get<std::unique_ptr<pgm>>(m_current);
			file << "P5" << '\n';
			write_common(file, c_pgm.common);
			file << c_pgm.max << '\n';
			write_data(file, c_pgm.common.data);
		}

		void write_ppm(std::ofstream& file, std::string_view path)
		{
			const auto& c_ppm = *std::get<std::unique_ptr<ppm>>(m_current);
			file << "P6" << '\n';
			write_common(file, c_ppm.common);
			file << c_ppm.max << '\n';
			write_data(file, c_ppm.common.data);
		}

		void write_common(std::ofstream& file, const common& common)
		{
			file << "# written by image::netpbm" << '\n';
			file << common.width << ' ' << common.height << '\n';
		}

		void write_data(std::ofstream& file, const std::vector<uint8_t>& data)
		{
			static_assert(sizeof(char) == 1);
			for (size_t i = 0; i < data.size(); i++)
				file.write(reinterpret_cast<const char*>(&data[i]), 1);
		}

	public:
		class exception : public image::exception
		{
		public:
			exception(std::string_view msg) :image::exception(msg) {}
			
			template<class... Args>
			static void enact(std::string_view format, Args&&... args)
			{
				throw exception(std::vformat(format, std::make_format_args(args...)));
			}
		};
	};
};
