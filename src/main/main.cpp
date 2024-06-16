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

	int exit_code = 0;
	try
	{
		using image::netpbm;

		logger log("main");

		netpbm::header_t header;
		size_t size = 0;
		netpbm::data_t data;

		if (false)
		{
			header.width = header.height = 1024;
			header.depth = 3;
			header.maxval = 255;
			header.tupltype = netpbm::tupltype_t::rgb;

			size = header.width * header.height * header.depth * 1;
			data = std::make_unique<uint8_t[]>(size);

			memset(data.get(), 0, size);
		}
		else
		{
			netpbm reader("assets/cross.pam");
			reader.save(&header, &size, &data);
		}

		auto setabs = [&] (int x, int y, uint32_t color) -> bool {
			if (x >= 0 && x < header.width && y >= 0 && y < header.height)
			{
				if (0)
				{
					const uint32_t bytes[3] = {
						color & 0xff,
						(color >> 8) & 0xff,
						(color >> 16) & 0xff
					};
					color = bytes[0] << 16 | bytes[1] << 8 | bytes[2];
				}

				const size_t location = (y * header.width + x) * 3;
				*reinterpret_cast<uint32_t*>(data.get() + location) = color;
				return true;
			}
			return false;
		};
		auto seti = [&] (int x, int y, uint32_t color) -> bool {
			const int half[2] = {header.width / 2, header.height / 2};
			return setabs(x + half[0], -y + half[1], color);
		};
		auto set = [&] (double x, double y, uint32_t color) -> bool {
			return seti(round(x), round(y), color);
		};

		auto getabs = [&] (int x, int y) -> uint32_t {
			if (x >= 0 && x < header.width && y >= 0 && y < header.height)
			{
				const size_t location = (y * header.width + x) * 3;
				return (uint32_t(data.get()[location]) << 0)
					| (uint32_t(data.get()[location+1]) << 8)
					| (uint32_t(data.get()[location+2]) << 16);
			}
			return 0;
		};
		auto geti = [&] (int x, int y) -> uint32_t {
			const int half[2] = {header.width / 2, header.height / 2};
			return getabs(x + half[0], -y + half[1]);
		};
		auto get = [&] (double x, double y) -> uint32_t {
			return geti(round(x), round(y));
		};

		auto rect = [&] (int bottom_left[2], int top_right[2], uint32_t color) {
			for (int y = bottom_left[1]; y < top_right[1]; y++)
			{
				for (int x = bottom_left[0]; x < top_right[0]; x++)
				{
					seti(x, y, color);
				}
			}
		};

		const double granularity = 1.0e-4;

		for (double m = -200; m <= 200; m += 0.25)
		for (double b = -header.width/2.0; b <= header.width/2.0; b += granularity)
		{
			double x = b;
			double y = x * m;
			// set(x, y, 0xff00ff);
		}

		// for (double r = 0.0; r <= header.width/2.0; r += 5.0)
		for (double b = 0.0; b < 2.0 * M_PI; b += granularity)
		{
			const double radius = 400.0;
			double x = cos(b) * radius;
			double y = sin(b) * radius;
			// seti(x, y, 0xffffff);
		}

		constexpr int each = 2;
		if (0)
		for (int y=0; y < header.height - each - 1; y += each)
		{
			for (int x = 0; x < header.width - each - 1; x += each)
			{
				uint32_t r[each * each];
				for (int iy = 0; iy < each; iy++)
					for (int ix = 0; ix < each; ix++)
						r[iy * each + ix] = getabs(ix + x, iy + y);

				uint32_t average = 0;
				for (int iy = 0; iy < each; iy++)
					for (int ix = 0; ix < each; ix++)
						average += r[iy * each + ix];
				average = 0xff'ff'ff'ff - average;
				// average += each * each;

				for (int iy = 0; iy < each; iy++)
					for (int ix = 0; ix < each; ix++)
						setabs(ix + x, iy + y, average);
			}
		}

		if (1)
		for (int y = 0; y < header.height; y++)
		{
			for (int x = 0; x < header.width; x++)
			{
				setabs(x, y, getabs(x, y));
			}
		}


		netpbm writer;
		writer.load(header, size, data);
		writer.save("/dev/shm/canvas.pam");
	}
	catch (image::netpbm::exception& e)
	{
		std::println(stderr, "Exception (image::netpbm::exception): {}", e.what());
		exit_code = 1;
	}
	catch (image::exception& e)
	{
		std::println(stderr, "Exception (image::exception): {}", e.what());
		exit_code = 1;
	}
	catch (std::exception& e)
	{
		std::println(stderr, "Exception (std::exception): {}", e.what());
		exit_code = 1;
	}

	return exit_code;
}
