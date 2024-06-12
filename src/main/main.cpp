import <string_view>;
import <print>;
import <vector>;
import <math.h>;
import <complex>;

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
			image::netpbm::ppm flavor {};
			flavor.max = 255;

			auto& common = flavor.common;
			common.width = common.height = 512;
			common.bpp = 24;
			common.bits = common.width * common.height * 24;
			common.data = std::vector<uint8_t>(common.bits / 8, 0);

			auto& data = common.data;

			auto set = [&] (int x, int y, uint32_t with) {
				if ((x >= 0 && x < common.width) && (y >= 0 && y < common.height))
				{
					const int land = (y * int(common.height) + x) * 3;
					data[land] = with & 0xff;
					data[land+1] = (with >> 8) & 0xff;
					data[land+2] = (with >> 16) & 0xff;
				}
			};
			auto set_center = [&] (int x, int y, uint32_t with) {
				const int center[2] = {int(common.width) / 2, int(common.height) / 2};
				set(center[0] + x, center[1] + -y, with);
			};

			for (float i = -(common.width/2.f); i < common.width/2.f; i += 1e-4f)
			{
				const float x = i;
				const float xx = x * 0.03f;
				const float y = sinf(xx) * cosf(xx * 10) * 100.f, y2 = sinf(xx) * 100.f;

				set_center(roundf(x), roundf(y), 0xf0'ff'0f);
				set_center(roundf(x), roundf(y2), 0xa0'ff'0f);
			}
			
			image.assign(flavor);
		}
		image.write("/dev/shm/canvas.ppm");
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
