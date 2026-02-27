#include <ProcessManager.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <sys/wait.h>
#include <vector>

ProcessManager::ProcessManager(const int maxProcesses)
	: m_maxProcesses(maxProcesses)
{
}

ProcessManager::~ProcessManager()
{
	WaitAll();
}

void ProcessManager::RunAsync(const std::string& command, const std::vector<std::string>& args, const std::string& outputFile)
{
	if (static_cast<int>(m_activeProcesses.size()) >= m_maxProcesses)
	{
		WaitOne();
	}
	m_activeProcesses.push_back(std::make_unique<Process>(command, args, outputFile));
}

void ProcessManager::RunSync(const std::string& command, const std::vector<std::string>& args)
{
	Process process(command, args);
	process.Wait();
}

void ProcessManager::WaitAll()
{
	while (!m_activeProcesses.empty())
	{
		WaitOne();
	}
}

void ProcessManager::WaitOne()
{
	if (m_activeProcesses.empty())
	{
		return;
	}

	if (pid_t pid = waitpid(-1, nullptr, 0); pid > 0)
	{
		const auto it = std::ranges::find_if(m_activeProcesses,
			[pid](const std::unique_ptr<Process>& process) { return process->GetPid() == pid; });

		if (it != m_activeProcesses.end())
		{
			(*it)->MarkWaited();
			m_activeProcesses.erase(it);
		}
	}
}