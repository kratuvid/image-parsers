import <string_view>;
import <print>;

import image;

int main()
{
	try
	{
		image::netpbm sample("assets/dragon-curve.ppm");
	}
	catch (image::netpbm::exception& e)
	{
		std::println(stderr, "image::netpbm::exception: {}", e.what());
	}
	catch (image::exception& e)
	{
		std::println(stderr, "image::exception: {}", e.what());
	}
	catch (std::exception& e)
	{
		std::println(stderr, "std::exception: {}", e.what());
	}
}
