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
	logger::global_init(logger::level::debug);
#else
	logger::global_init();
#endif

	try
	{
		logger main_logger("main");

		image::netpbm image;

		{
			image::netpbm::ppm flavor {};
			flavor.max = 255;

			auto& common = flavor.common;
			common.width = common.height = 768;
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

			// for (float x = -(common.width/2.f); x < common.width/2.f; x += 1e-4f)
			for (float i = 0.f; i < 32.f * M_PI; i += 1e-5f)
			{
				const float x = cosf(i * 3) * (100.f - i);
				const float y = sinf(i) * (100.f + i);

				set_center(roundf(x), roundf(y), 0xf0'ff'0f);
			}
			
			image.assign(flavor);
		}

		image.write("/dev/shm/canvas.ppm");
	}
	catch (image::netpbm::exception& e)
	{
		std::println(stderr, "Exception (image::netpbm::exception): {}", e.what());
	}
	catch (image::exception& e)
	{
		std::println(stderr, "Exception (image::exception): {}", e.what());
	}
	catch (std::exception& e)
	{
		std::println(stderr, "Exception (std::exception): {}", e.what());
	}
}
