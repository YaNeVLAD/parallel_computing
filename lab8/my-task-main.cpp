#include <Task.hpp>

#include <iostream>

Task<std::string> SimpleCoroutine()
{
	co_return "Hello from coroutine!";
}

int main()
{
	const auto task = SimpleCoroutine();
	std::cout << task.Result() << std::endl;

	return 0;
}