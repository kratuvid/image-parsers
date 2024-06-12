export namespace image
{
	class netpbm
	{
		struct common {
			unsigned width, height;
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
		using variant_t = std::variant<pbm, pgm, ppm>;

		logger m_logger {"image:netpbm"};
		std::unique_ptr<variant_t> m_current;
		
	public:
		netpbm(std::string_view path)
		{
			load(path);
		}

		void load(std::string_view path)
		{
			std::ifstream file(path.data());
			if (!file.is_open())
				exception::enact("Couldn't open '{}'", path);

			m_logger.debug("Attempting to open '{}'", path);
		}

	private:
		void load_pbm(std::ifstream& file)
		{
		}

		void load_pgm(std::ifstream& file)
		{
		}

		void load_ppm(std::ifstream& file)
		{
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
