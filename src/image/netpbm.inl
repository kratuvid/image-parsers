export namespace image
{
	class netpbm
	{
	public:
		netpbm(std::string_view file)
		{
		}

	public:
		class exception : public image::exception
		{
		public:
			exception(std::string_view msg) :image::exception(msg) {}
			
			template<class... Args>
			void enact(std::string_view format, Args&&... args)
			{
				image::exception::enact<exception>("netpbm", format, args...);
			}
		};
	};
};
