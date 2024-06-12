import <string_view>;
import <print>;
import <vector>;
import <math.h>;

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
		image::netpbm image;
		{
			image::netpbm::pgm flavor {};
			flavor.max = 255;

			auto& common = flavor.common;
			common.width = 512; common.height = 512;
			common.bpp = 8;
			common.bits = common.width * common.height * 8;
			common.data = std::vector<uint8_t>(common.bits / 8, 0);

			auto& data = common.data;

			auto set = [&] (int x, int y, uint8_t with) {
				const int land = y * int(common.height) + x;
				if (land >= 0 and land < data.size())
					data[land] = with;
			};
			auto set_center = [&] (int x, int y, uint8_t with) {
				const int center[2] = {int(common.width) / 2, int(common.height) / 2};
				set(center[0] + x, center[1] + y, with);
			};
			
			for (float i = 0; i < 360; i += 0.01f)
			{
				const float radius = 128;
				set_center(cosf(i) * radius, sinf(i) * radius, 0xff);
			}
			
			image.assign(flavor);
		}
		image.write("/dev/shm/canvas.pgm");
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
