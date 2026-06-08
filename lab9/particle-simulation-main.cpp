#define CL_HPP_TARGET_OPENCL_VERSION 300
#define CL_HPP_ENABLE_EXCEPTIONS

#include <CL/opencl.hpp>

#include <iostream>
#include <string>
#include <vector>

#include <Console.hpp>

int main()
{
	pc::Console::SetLocale(pc::Locale::UTF8);

	try
	{
		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);

		if (platforms.empty())
		{
			std::cerr << "Платформы OpenCL не найдены. Проверьте установку драйверов." << '\n';
			return 1;
		}

		std::cout << "Найдено платформ: " << platforms.size() << "\n\n";

		for (const auto& platform : platforms)
		{
			std::cout << "Платформа: " << platform.getInfo<CL_PLATFORM_NAME>() << '\n';
			std::cout << "Вендор:   " << platform.getInfo<CL_PLATFORM_VENDOR>() << '\n';

			std::vector<cl::Device> devices;
			platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
			for (const auto& device : devices)
			{
				std::cout << "  - Устройство: " << device.getInfo<CL_DEVICE_NAME>() << '\n';
				std::cout << "    Версия OpenCL: " << device.getInfo<CL_DEVICE_VERSION>() << '\n';
			}
			std::cout << "----------------------------------------\n";
		}
	}
	catch (const cl::Error& err)
	{
		std::cerr << "Ошибка OpenCL: " << err.what() << " (" << err.err() << ")\n";
		return 1;
	}

	return 0;
}