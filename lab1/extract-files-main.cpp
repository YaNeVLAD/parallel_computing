#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include <ProcessManager.hpp>

namespace fs = std::filesystem;

int main(const int argc, char* argv[])
{
	if (argc < 4)
	{
		std::cerr << "Usage:\n"
				  << "  extract-files -S ARCHIVE-NAME OUTPUT-FOLDER\n"
				  << "  extract-files -P NUM-PROCESSES ARCHIVE-NAME OUTPUT-FOLDER\n";
		return 1;
	}

	const auto startTotal = std::chrono::steady_clock::now();

	int maxProcesses = 1;

	const std::string modeFlag = argv[1];
	int argIndex = 2;

	if (modeFlag == "-P")
	{
		try
		{
			maxProcesses = std::stoi(argv[argIndex++]);
		}
		catch (...)
		{
			std::cerr << "Invalid NUM-PROCESSES argument\n";
			return 1;
		}
	}
	else if (modeFlag != "-S")
	{
		std::cerr << "Unknown mode: " << modeFlag << "\n";
		return 1;
	}

	std::string archiveName = argv[argIndex++];
	std::string outputFolder = argv[argIndex++];

	fs::create_directories(outputFolder);

	ProcessManager processManager(maxProcesses);

	processManager.RunSync("tar", { "-xf", archiveName, "-C", outputFolder });

	for (const auto& entry : fs::recursive_directory_iterator(outputFolder))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".gz")
		{
			processManager.RunAsync("gzip", { "-d", entry.path().string() });
		}
	}

	processManager.WaitAll();

	const auto endTotal = std::chrono::steady_clock::now();
	const std::chrono::duration<double> totalTime = endTotal - startTotal;

	std::cout << "Total time: " << totalTime.count() << " seconds\n";

	return 0;
}