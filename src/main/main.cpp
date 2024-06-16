import <cstring>;
import <math.h>;
import <memory>;
import <print>;
import <string_view>;

import image;
import logger;

int main(int argc, char** argv)
{
	logger::global_init(logger::level::debug);

	try
	{
		using image::netpbm;

		logger log("main");

		netpbm::header_t header;
		size_t size = 0;
		netpbm::data_t data;

		header.width = header.height = 768;
		header.depth = 3;
		header.maxval = 255;
		header.tupltype = netpbm::tupltype_t::rgb;

		size = header.width * header.height * header.depth * 1;
		data = std::make_unique<uint8_t[]>(size);

		auto setabs = [&] (int x, int y, uint32_t color) {
			if (x >= 0 and x < header.width and y >= 0 and y < header.height)
			{
				const uint32_t lower = color & 0xff;
				color &= 0xffff00;
				color |= (color >> 16) & 0xff;
				color &= 0x00ffff;
				color |= lower << 16;

				const size_t location = (y * header.width + x) * 3;
				*(reinterpret_cast<uint32_t*>(data.get() + location)) = color;
			}
		};

		auto set = [&] (int x, int y, uint32_t color) {
			const int half[2] = {header.width / 2, header.height / 2};
			setabs(x + half[0], -y + half[1], color);
		};

		auto setf = [&] (float x, float y, uint32_t color) {
			set(roundf(x), roundf(y), color);
		};

		auto rect = [&] (int bottom_left[2], int top_right[2], uint32_t color) {
			for (int y = bottom_left[1]; y < top_right[1]; y++)
			{
				for (int x = bottom_left[0]; x < top_right[0]; x++)
				{
					set(x, y, color);
				}
			}
		};

		const float granularity = 1e-2f;
		for (float b = -header.width/2.f; b < header.width/2.f; b += granularity)
		{
			float x = b;
			float y = x * 2;
			setf(x, y, 0xff0000);
		}

		netpbm writer;
		writer.load(header, size, data);
		writer.save("/dev/shm/canvas.pam");
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
