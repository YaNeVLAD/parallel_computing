#include <Application.hpp>

#include <iostream>

int main()
{
	using namespace pc::simulation;

	try
	{
		Application app;
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Critical Error: " << e.what() << '\n';
		return -1;
	}

	return 0;
}