import <string_view>;
import <print>;

import logger;
import image;

int main()
{
#ifdef DEBUG
	logger::init(logger::level::debug);
#else
	logger::init();
#endif

	logger main_logger("main");
	
	try
	{
		const std::initializer_list<std::string_view> test_files = {
			"assets/fractal-dragon-curve.pbm",
			"assets/j_bin.pbm",

			"assets/fractal-dragon-curve.pgm",
			"assets/feep.pgm",
			"assets/feep_bin.pgm",

			"assets/fractal-dragon-curve.ppm",
			"assets/colors.ppm",
			"assets/colors_bin.ppm"
		};

		for (const auto& path : test_files)
		{
			image::netpbm parser(path);
			if (path.ends_with(".pbm"))
				parser.write(std::format("out/{}", path));
			else if (path.ends_with(".pgm"))
				parser.write(std::format("out/{}", path));
			else
				parser.write(std::format("out/{}", path));
		}
	}
	catch (image::netpbm::exception& e)
	{
		main_logger.critical("image::netpbm::exception: {}", e.what());
	}
	catch (image::exception& e)
	{
		main_logger.critical("image::exception: {}", e.what());
	}
	catch (std::exception& e)
	{
		main_logger.critical("std::exception: {}", e.what());
	}
}
