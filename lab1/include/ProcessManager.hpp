#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Process.hpp>

class ProcessManager
{
public:
	explicit ProcessManager(int maxProcesses);
	~ProcessManager();

	void RunAsync(const std::string& command, const std::vector<std::string>& args, const std::string& outputFile = "");
	void RunSync(const std::string& command, const std::vector<std::string>& args);
	void WaitAll();

private:
	void WaitOne();

	int m_maxProcesses;
	std::vector<std::unique_ptr<Process>> m_activeProcesses;
};