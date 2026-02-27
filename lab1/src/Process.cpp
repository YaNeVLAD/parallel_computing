#include <Process.hpp>

#include <fcntl.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

Process::Process(const std::string& cmd, const std::vector<std::string>& args, const std::string& outPath)
	: m_isWaited(false)
{
	m_pid = fork();
	if (m_pid < 0)
	{
		throw std::runtime_error("Failed to fork process");
	}
	else if (m_pid == 0)
	{
		if (!outPath.empty())
		{
			const int fd = open(outPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (fd < 0)
			{
				std::cerr << "Failed to open output file: " << outPath << "\n";
				exit(1);
			}
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}

		std::vector<char*> cArgs;
		cArgs.push_back(const_cast<char*>(cmd.c_str()));
		for (const auto& arg : args)
		{
			cArgs.push_back(const_cast<char*>(arg.c_str()));
		}
		cArgs.push_back(nullptr);

		execvp(cmd.c_str(), cArgs.data());
		std::cerr << "Failed to execute command: " << cmd << "\n";
		exit(1);
	}
}

Process::~Process()
{
	Wait();
}

Process::Process(Process&& other) noexcept
	: m_pid(other.m_pid)
	, m_isWaited(other.m_isWaited)
{
	other.m_pid = -1;
	other.m_isWaited = true;
}

Process& Process::operator=(Process&& other) noexcept
{
	if (this != &other)
	{
		Wait();
		m_pid = other.m_pid;
		m_isWaited = other.m_isWaited;

		other.m_pid = -1;
		other.m_isWaited = true;
	}
	return *this;
}

void Process::Wait()
{
	if (!m_isWaited && m_pid > 0)
	{
		waitpid(m_pid, nullptr, 0);
		m_isWaited = true;
	}
}

pid_t Process::GetPid() const
{
	return m_pid;
}

void Process::MarkWaited()
{
	m_isWaited = true;
}