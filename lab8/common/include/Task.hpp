#pragma once

#include <coroutine>
#include <exception>
#include <utility>

template <typename T>
class Task
{
public:
	struct promise_type
	{
		T returnValue;

		Task get_return_object() { return Task{ std::coroutine_handle<promise_type>::from_promise(*this) }; }

		std::suspend_never initial_suspend() noexcept { return {}; }

		std::suspend_always final_suspend() noexcept { return {}; }

		void return_value(T value) { returnValue = std::move(value); }

		void unhandled_exception() { std::terminate(); }
	};

	Task() = default;

	explicit Task(std::coroutine_handle<promise_type> handle)
		: m_handle(handle)
	{
	}

	Task(const Task&) = delete;
	Task& operator=(const Task&) = delete;

	Task(Task&& other) noexcept
		: m_handle(std::exchange(other.m_handle, nullptr))
	{
	}

	Task& operator=(Task&& other) noexcept
	{
		if (this != &other)
		{
			if (m_handle)
			{
				m_handle.destroy();
			}
			m_handle = std::exchange(other.m_handle, nullptr);
		}

		return *this;
	}

	~Task()
	{
		if (m_handle)
		{
			m_handle.destroy();
		}
	}

	void Resume()
	{
		if (m_handle && !m_handle.done())
		{
			m_handle.resume();
		}
	}

	T Result() const
	{
		return m_handle.promise().returnValue;
	}

private:
	std::coroutine_handle<promise_type> m_handle;
};