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
		// image::netpbm sample("assets/fractal-dragon-curve.pbm");
		image::netpbm sample("assets/short.pbm"), sample2("assets/short_binary.pbm"),
			sample3("assets/fractal-dragon-curve.pbm");
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
