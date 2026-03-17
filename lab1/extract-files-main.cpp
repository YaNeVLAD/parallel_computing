#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include <ProcessManager.hpp>

namespace fs = std::filesystem;

struct ProgramArgs
{
	int maxProcesses = -1;
	bool isValid = false;

	std::string archiveName{};
	std::string outputFolder{};
};

void PrintUsage()
{
	std::cerr << "Usage:\n"
			  << "  extract-files -S ARCHIVE-NAME OUTPUT-FOLDER\n"
			  << "  extract-files -P NUM-PROCESSES ARCHIVE-NAME OUTPUT-FOLDER\n";
}

ProgramArgs ParseArguments(const int argc, char* argv[])
{
	ProgramArgs args;
	if (argc < 4)
	{
		return args;
	}

	const std::string modeFlag = argv[1];
	int argIndex = 2;

	if (modeFlag == "-P")
	{
		try
		{
			args.maxProcesses = std::stoi(argv[argIndex++]);
		}
		catch (...)
		{
			std::cerr << "Error: Invalid NUM-PROCESSES argument\n";
			return args;
		}
	}
	else if (modeFlag != "-S")
	{
		std::cerr << "Error: Unknown mode: " << modeFlag << "\n";
		return args;
	}

	if (argIndex + 1 >= argc)
	{
		return args;
	}

	args.archiveName = argv[argIndex++];
	args.outputFolder = argv[++argIndex];
	args.isValid = true;

	return args;
}

std::chrono::duration<double>
UnpackArchive(ProcessManager& pm, const std::string& archive, const std::string& target)
{
	fs::create_directories(target);

	const auto start = std::chrono::steady_clock::now();
	pm.RunSync("tar", { "-xf", archive, "-C", target });
	const auto end = std::chrono::steady_clock::now();

	return end - start;
}

void DecompressFiles(ProcessManager& pm, const std::string& folder)
{
	for (const auto& entry : fs::recursive_directory_iterator(folder))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".gz")
		{
			pm.RunAsync("gzip", { "-d", entry.path().string() });
		}
	}
	pm.WaitAll();
}

int main(const int argc, char* argv[])
{
	const auto startTotal = std::chrono::steady_clock::now();

	auto [maxProcesses, isValid, archiveName, outputFolder] = ParseArguments(argc, argv);
	if (!isValid)
	{
		PrintUsage();
		return 1;
	}

	ProcessManager processManager(maxProcesses);

	const auto tarDuration = UnpackArchive(processManager, archiveName, outputFolder);

	DecompressFiles(processManager, outputFolder);

	const auto endTotal = std::chrono::steady_clock::now();
	const std::chrono::duration<double> totalDuration = endTotal - startTotal;

	std::cout << "Total time: " << totalDuration.count() << " seconds\n";
	std::cout << "Sequential (tar) time: " << tarDuration.count() << " seconds\n";

	return 0;
}