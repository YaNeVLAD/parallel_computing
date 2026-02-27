#pragma once

#include <string>
#include <vector>

class Process
{
public:
	Process(const std::string& cmd, const std::vector<std::string>& args, const std::string& outPath = "");
	~Process();

	Process(const Process&) = delete;
	Process& operator=(const Process&) = delete;

	Process(Process&& other) noexcept;
	Process& operator=(Process&& other) noexcept;

	void Wait();
	void MarkWaited();

	[[nodiscard]] pid_t GetPid() const;

private:
	pid_t m_pid;
	bool m_isWaited;
};
