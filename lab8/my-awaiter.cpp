#include <Awaiter.hpp>
#include <Task.hpp>

#include <iostream>

Task<int> CoroutineWithAwait(const int x, const int y)
{
	std::cout << "Before await\n";
	const int result = co_await Awaiter{ x, y };
	std::cout << result << "\n";
	std::cout << "After await\n";

	co_return 42;
}

int main()
{
	auto task = CoroutineWithAwait(10, 20);
	std::cout << "Before resume\n";
	task.Resume();
	std::cout << "After resume\n";

	CoroutineWithAwait(5, 10).Resume();

	std::cout << "End of main\n";

	return 0;
}