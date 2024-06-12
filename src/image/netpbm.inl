export namespace image
{
	class netpbm
	{
		struct common {
			size_t width, height;
			size_t bits;
			std::vector<uint8_t> data;
		};
		struct pbm {
			struct common common;
		};
		struct pgm {
			struct common common;
		};
		struct ppm {
			struct common common;
		};

		logger m_logger {"image:netpbm"};

		std::variant<std::unique_ptr<pbm>, std::unique_ptr<pgm>, std::unique_ptr<ppm>> m_current;
		
	public:
		netpbm(std::string_view path)
		{
			load(path);
		}

		void load(std::string_view path)
		{
			std::ifstream file(path.data(), std::ios::binary);
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
			m_logger.debug("Header: PBM ({}), width: {}, height: {}", binary ? "binary" : "ascii", c_pbm.common.width, c_pbm.common.height);
			if (c_pbm.common.width == 0 || c_pbm.common.height == 0)
				exception::enact("Invalid width/height ({}/{}) specified in '{}'", c_pbm.common.width, c_pbm.common.height);

			const size_t bits = c_pbm.common.bits = c_pbm.common.width * c_pbm.common.height;
			const size_t bytes = bits % 8 == 0 ? bits / 8 : (bits / 8) + 1;
			const size_t bytes_to_read = binary ? bytes : bits;

			const auto prev_at = file.tellg();
			file.seekg(0, std::ios::end);
			const size_t bytes_available = file.tellg() - prev_at;
			file.seekg(prev_at);

			if (bytes_to_read > bytes_available)
				exception::enact("{}B of data missing in '{}'", bytes_to_read - bytes_available);

			c_pbm.common.data.resize(bytes_to_read);
			memset(c_pbm.common.data.data(), 0, bytes);

			static_assert(sizeof(char) == 1);
			char buffer[64];
			size_t read = 0;
			for (size_t at = 0; at < bytes_to_read; at += file.gcount())
			{
				if (!file || file.eof())
					exception::enact("Failed to read the rest of the {}B in the data section in '{}'", bytes_to_read - at, path);

				const size_t bytes_do = std::min(sizeof(buffer), bytes_to_read - at);
				file.read(buffer, bytes_do);
				memcpy(c_pbm.common.data.data() + at, buffer, bytes_do);
			}

			if (!binary)
			{
				size_t source_at = 0, dest_at = 0;
				for (; source_at < bytes_to_read; source_at += 8, dest_at++)
				{
					uint8_t source = 0;

					const size_t bytes_do = std::min(size_t(8), bytes_to_read - source_at);
					for (size_t i = source_at; i < bytes_do; i++)
					{
						const uint8_t bit = '1' - c_pbm.common.data[source_at];
						source |= bit << i;
					}

					c_pbm.common.data[dest_at] = source;
				}

				c_pbm.common.data.resize(bytes);
				c_pbm.common.data.shrink_to_fit();
			}

			m_logger.debug("Size: struct: {}B, data: {}B = {}B", sizeof(pbm), c_pbm.common.data.size(), sizeof(pbm) + c_pbm.common.data.size());
		}

		void load_pgm(std::ifstream& file, bool binary, std::string_view path)
		{
		}

		void load_ppm(std::ifstream& file, bool binary, std::string_view path)
		{
			m_logger.debug("A color {} PPM", binary ? "binary" : "ascii");
		}

	private:
		

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
