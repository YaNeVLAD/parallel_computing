#pragma once

#include <coroutine>

struct Awaiter
{
	int x;
	int y;

	bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<>) const noexcept {}

	int await_resume() const noexcept { return x + y; }
};